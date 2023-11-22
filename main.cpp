#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"
#include "jit_init.h"
#include "sql_peg.h"
#include "windows_manager.h"
#include "llvm/ADT/Triple.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/ExecutionEngine/Orc/ObjectTransformLayer.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/ValueSymbolTable.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCDisassembler/MCDisassembler.h"
#include "llvm/MC/MCInstPrinter.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include <any>
#include <iostream>
using namespace ToySQLEngine;
using namespace ToySQLEngine::SQLParser;
using namespace llvm;
using namespace llvm::orc;
using namespace llvm::detail;
ExitOnError ExitOnErr;

struct table {
    int64_t a;
    int64_t b;
};

struct Disassembler {
    Disassembler() {
        Triple triple;
        std::string error;
        auto const *target = TargetRegistry::lookupTarget("x86-64", triple, error);
        if (target == nullptr)
            throw std::runtime_error(error);

        MCTargetOptions options;
        const auto instr = target->createMCInstrInfo();
        const llvm::MCRegisterInfo *register_info = target->createMCRegInfo(triple.str());
        const auto asm_info = target->createMCAsmInfo(*register_info, triple.str(), options);
        printer_ = target->createMCInstPrinter(triple, 0, *asm_info, *instr, *register_info);
        sub_ = target->createMCSubtargetInfo(triple.str(), "", "");

        context_ = std::make_unique<MCContext>(triple, asm_info, register_info, sub_);
        disassembler_ = target->createMCDisassembler(*sub_, *context_);
    }

    template<typename T>
    void disassembler(uint64_t addr, T &&buffer) {
        llvm::MCInst inst;
        uint64_t size;
        const auto base_address = addr;
        printer_->setPrintBranchImmAsAddress(true);
        ArrayRef<uint8_t> bytearray(buffer);
        while (disassembler_->getInstruction(inst, size, bytearray.slice(addr - base_address), addr, llvm::nulls())) {
            std::string dis_string;
            llvm::raw_string_ostream ss(dis_string);
            printer_->printInst(&inst, addr, "", *sub_, ss);

            std::stringstream stream;
            stream << std::hex << addr;
            inst_string_ += "[";
            inst_string_ += stream.str();
            inst_string_ += "] ";
            inst_string_ += dis_string;
            inst_string_ += '\n';
            if (dis_string.find("ret") != std::string::npos)
                break;
            addr += size;
        }
    }

    std::string get_asm() { return inst_string_; }

private:
    Target *target_ = nullptr;
    MCDisassembler *disassembler_;
    std::unique_ptr<llvm::MCContext> context_;
    llvm::MCInstPrinter *printer_;
    MCSubtargetInfo *sub_;
    std::string inst_string_;
};


class ExecuteWindow : public WindowsLayer<WindowsManager> {
public:
    explicit ExecuteWindow(WindowsManager &manager, std::unique_ptr<LLJIT> &lljit, const std::vector<table> &table) : lljit_(lljit), table_(table), WindowsLayer<WindowsManager>(manager) {
    }
    void Draw() override {

        ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("TableWindow");
        ImGui::InputText("SQL Input", &sql_);
        ImGui::SameLine();
        if (ImGui::Button("Compile")) {
            this->Compile(sql_);
        }

        if (ast_ != nullptr && ImGui::CollapsingHeader("AstTree")) {
            if (ImGui::TreeNode("Root")) {
                for (const auto &it: ast_->children) {
                    this->DumpAst(it);
                }
                ImGui::TreePop();
            }
        }

        if (!ir_.empty() && ImGui::CollapsingHeader("LLVM IR")) {
            ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(this->ir_.data());
            ImGui::EndChild();
        }

        if (!asm_.empty() && ImGui::CollapsingHeader("Assembly")) {
            ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::TextUnformatted(this->asm_.data());
            ImGui::EndChild();
        }
        ImGui::End();
    }

    template<typename T>
    void DumpAst(T &ast, int idx = 0, int depth = 0) {
        auto label = std::string(ast->type) + "/" + std::to_string(depth) + "/" + std::to_string(idx);
        if (ImGui::TreeNode(label.data())) {
            if (ast->has_content()) {
                ImGui::Text("[text]%s", ast->string().data());
            }
            int i = 0;
            for (const auto &it: ast->children) {
                this->DumpAst(it, i, depth + 1);
                i++;
            }
            ImGui::TreePop();
        }
    }

    ~ExecuteWindow() override = default;

    void Compile(std::string_view sql) {
        if (rt_ != nullptr)
            ExitOnErr(this->rt_->get()->remove());

        std::unique_ptr<LLVMContext> llvm_context_ = std::make_unique<LLVMContext>();
        SMDiagnostic dia;
        auto jit_module_ = getLazyIRFileModule(JIT_INIT_PATH, dia, *llvm_context_);
        if (jit_module_ == nullptr) {
            throw std::runtime_error(dia.getMessage().str());
        }

        ToySQLEngine::SQLParser::memory_input in(sql.data(), "");
        ast_ = ToySQLEngine::SQLParser::parse_tree::parse<ToySQLEngine::SQLParser::select_stmt, ToySQLEngine::SQLParser::token_selector>(in);
        ToySQLEngine::SQLParser::RestructureNode restructure{};
        restructure(ast_);

        StructType *table_column_info_type = StructType::getTypeByName(*llvm_context_, "struct.table_column_info");
        jit_module_->getOrInsertGlobal("table_info", ArrayType::get(table_column_info_type, 2));
        auto g_table_info = jit_module_->getNamedGlobal("table_info");
        g_table_info->setDSOLocal(true);

        std::string col_str = "a";
        col_str.resize(128);
        auto *value = ConstantStruct::get(table_column_info_type,
                                          ConstantDataArray::getRaw(col_str, 128, Type::getInt8Ty(*llvm_context_)),
                                          ConstantInt::get(Type::getInt32Ty(*llvm_context_), 0),
                                          ConstantInt::get(Type::getInt32Ty(*llvm_context_), 0));

        col_str = "b";
        auto *b_value = ConstantStruct::get(table_column_info_type,
                                            ConstantDataArray::getRaw(col_str, 128, Type::getInt8Ty(*llvm_context_)),
                                            ConstantInt::get(Type::getInt32Ty(*llvm_context_), 2),
                                            ConstantInt::get(Type::getInt32Ty(*llvm_context_), 8));

        g_table_info->setInitializer(ConstantArray::get(ArrayType::get(table_column_info_type, 2), {value, b_value}));
        g_table_info->setConstant(true);

        auto col_a_str = ConstantDataArray::getString(*llvm_context_, "a");
        jit_module_->getOrInsertGlobal("str#a", col_a_str->getType());
        auto col_a = jit_module_->getNamedGlobal("str#a");
        col_a->setConstant(true);
        col_a->setDSOLocal(true);
        col_a->setInitializer(col_a_str);

        auto col_b_str = ConstantDataArray::getString(*llvm_context_, "b");
        jit_module_->getOrInsertGlobal("str#b", col_a_str->getType());
        auto col_b = jit_module_->getNamedGlobal("str#b");
        col_b->setConstant(true);
        col_b->setDSOLocal(true);
        col_b->setInitializer(col_b_str);

        query_func_ = Function::Create(FunctionType::get(Type::getInt1Ty(*llvm_context_), {Type::getInt8PtrTy(*llvm_context_)}, false), Function::ExternalLinkage, "query", jit_module_.get());
        BasicBlock *bb = BasicBlock::Create(*llvm_context_, "entry", query_func_);
        IRBuilder<> builder(bb);

        g_table_info_ = builder.CreateBitCast(jit_module_->getGlobalVariable("table_info"), PointerType::get(table_column_info_type, 0), "table_info_ptr");

        true_block_ = BasicBlock::Create(*llvm_context_, "ret_true", query_func_);
        false_block_ = BasicBlock::Create(*llvm_context_, "ret_false", query_func_);

        builder.SetInsertPoint(true_block_);
        builder.CreateRet(ConstantInt::get(Type::getInt1Ty(*llvm_context_), 1));
        builder.SetInsertPoint(false_block_);
        builder.CreateRet(ConstantInt::get(Type::getInt1Ty(*llvm_context_), 0));

        builder.SetInsertPoint(bb);
        GenCode(ast_, builder, bb, jit_module_);

        builder.CreateCondBr(any_cast<Value *>(stack_.back()), true_block_, false_block_);
        stack_.pop_back();
        stack_.clear();

        rt_ = std::make_unique<IntrusiveRefCntPtr<ResourceTracker>>(lljit_->getMainJITDylib().createResourceTracker());
        ir_.clear();
        llvm::raw_string_ostream ss(ir_);
        jit_module_->print(ss, nullptr);
        ExitOnErr(lljit_->addIRModule(*rt_, ThreadSafeModule(std::move(jit_module_), std::move(llvm_context_))));
        auto addr = ExitOnErr(lljit_->lookup("query"));
        ArrayRef<uint8_t> ref(addr.toPtr<const unsigned char *>(), 1024);
        Disassembler d;
        d.disassembler(addr.getValue(), ref);
        asm_ = d.get_asm();
    }

    bool execute(void *ptr) {
        auto addr = ExitOnErr(lljit_->lookup("query"));
        return addr.toPtr<bool(void *)>()(ptr);
    }

private:
    void GenCode(const std::unique_ptr<ToySQLEngine::SQLParser::Node> &node, IRBuilder<> &builder, BasicBlock *bb, std::unique_ptr<Module> &jit_module_, size_t depth = 0) {
        if (depth > 128)
            throw std::runtime_error("Maximum recursion depth");

        const auto bb_sym_table = bb->getValueSymbolTable();
        if (node->is_type<where_condition>()) {
            const auto &col_name = node->children[0];
            assert(col_name->is_type<column_name>());
            Function *query_table_column_func = jit_module_->getFunction("query_table_column");
            auto column_name_ptr = builder.CreateBitCast(jit_module_->getNamedGlobal("str#" + col_name->string()), Type::getInt8PtrTy(jit_module_->getContext()));
            auto query_call = builder.CreateCall(query_table_column_func, {g_table_info_, ConstantInt::get(Type::getInt32Ty(jit_module_->getContext()), 2), column_name_ptr});
            auto *get_table_column_int64_func = jit_module_->getFunction("get_table_column_int64");
            assert(get_table_column_int64_func != nullptr);
            auto current_val = builder.CreateCall(get_table_column_int64_func, {query_call, query_func_->arg_begin()});

            const auto &v = node->children[1]->children.begin();
            const auto &op = node->children[2];
            if (v->get()->is_type<integer>()) {
                auto i64_v = ConstantInt::get(Type::getInt64Ty(jit_module_->getContext()), std::stoll(v->get()->string_view().data()));
                Value *ret = nullptr;
                if (op->string_view() == "=") {
                    ret = builder.CreateICmpEQ(current_val, i64_v);
                } else if (op->string_view() == "<>" || op->string_view() == "!=") {
                    ret = builder.CreateICmpNE(current_val, i64_v);
                } else if (op->string_view() == "<") {
                    ret = builder.CreateICmpSLT(current_val, i64_v);
                } else if (op->string_view() == ">") {
                    ret = builder.CreateICmpSGT(current_val, i64_v);
                } else if (op->string_view() == ">=") {
                    ret = builder.CreateICmpSGE(current_val, i64_v);
                } else if (op->string_view() == "<=") {
                    ret = builder.CreateICmpSLE(current_val, i64_v);
                }
                stack_.emplace_back(ret);
            }
        }

        if (node->is_type<where_logic>()) {
            auto r = any_cast<Value *>(stack_.back());
            stack_.pop_back();
            auto l = any_cast<Value *>(stack_.back());
            stack_.pop_back();
            if (node->string_view() == "and") {
                stack_.emplace_back(builder.CreateAnd(r, l));
            } else if (node->string_view() == "or") {
                stack_.emplace_back(builder.CreateOr(r, l));
            }
        }
        for (const auto &it: node->children) {
            this->GenCode(it, builder, bb, jit_module_, depth + 1);
        }
    }

    decltype(ExitOnErr(LLJITBuilder().create())) jit_builder_ = ExitOnErr(LLJITBuilder().create());
    std::vector<std::any> stack_;
    Function *query_func_ = nullptr;
    StringMap<std::vector<table_column_info>> table_info;
    BasicBlock *true_block_{};
    BasicBlock *false_block_{};
    Value *g_table_info_{};
    std::unique_ptr<LLJIT> &lljit_;
    std::unique_ptr<IntrusiveRefCntPtr<ResourceTracker>> rt_;
    std::string asm_;
    const std::vector<table> &table_;
    std::string sql_ = "select * from table where a = 1 and b = 2";
    std::unique_ptr<SQLParser::parse_tree::node> ast_ = nullptr;
    std::string ir_;
};


int main(int argc, const char **argv) {
    InitializeNativeTarget();
    InitializeNativeTargetDisassembler();
    InitializeNativeTargetAsmParser();
    InitializeNativeTargetAsmPrinter();
    auto jit = ExitOnErr(LLJITBuilder().create());

    std::vector<table> table_data;
    for (auto i = 0; i < 100; i++) {
        for (auto j = 0; j < 100; j++) {
            table_data.emplace_back(table{i, j});
        }
    }

    try {
        WindowsManager manager(0, 0);
        WindowsManager::SetFont(FONT_PATH, 21);
        manager.CreateLayer<ExecuteWindow>(jit, table_data);
        manager.Run();
    } catch (const ToySQLEngine::SQLParser::parse_error &e) {
        const auto p = e.positions().front();
        std::cerr << e.what() << std::endl;
    }

    return 0;
}