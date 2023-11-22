#pragma once
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <tao/pegtl/contrib/parse_tree_to_dot.hpp>

namespace ToySQLEngine::SQLParser {
    using namespace tao::pegtl;
    using Node = parse_tree::node;
    // SQL Grammar
    //SELECT_STMT <- SELECT_TOKEN SELECT_COLUMN SELECT_FROM WHERE_STMT?
    //SELECT_TOKEN <- 'select' / 'SELECT'
    //SELECT_COLUMN <- '*'
    //SELECT_FROM <- ('from' / 'FROM') TABLE_NAME SPACE*
    //WHERE_STMT <- ('where' / 'WHERE') (WHERE_GROUP / WHERE_GROUP_PRIORITY)+
    //WHERE_AND <- ('and' / 'AND')
    //WHERE_OR <- ('or' / 'OR')
    //WHERE_LOGIC <- WHERE_AND / WHERE_OR
    //WHERE_OP <- '=' / '!=' / '<>' / '<' / '<=' / '>' / '>='
    //NUMBER <- [0-9]+ ('.'[0-9]*)?
    //WHERE_VALUE <- NUMBER
    //WHERE_GROUP_PRIORITY <- '('SPACE* (WHERE_GROUP_PRIORITY/WHERE_GROUP) SPACE* ')'
    //WHERE_GROUP <- WHERE_CONDITION (SPACE+ WHERE_LOGIC (WHERE_CONDITION / WHERE_GROUP_PRIORITY))*
    //WHERE_CONDITION <- COLUMN_NAME SPACE? WHERE_OP SPACE? WHERE_VALUE
    //TABLE_NAME <- ([a-zA-Z])+
    //COLUMN_NAME <- ([a-zA-Z])+
    //SPACE <- [ \t\n\r]
    //%whitespace <- [ \t\n]*)
    struct white_space : space {};
    struct integer : seq<opt<one<'-'>>, plus<digit>> {};
    struct floating : seq<integer, one<'.'>, plus<digit>> {};
    struct column_value : sor<floating, integer> {};
    struct column_name : plus<ranges<'a', 'z', 'A', 'Z'>> {};
    struct table_name : column_name {};
    struct where_and : TAO_PEGTL_ISTRING("and") {};
    struct where_or : TAO_PEGTL_ISTRING("or") {};
    struct where_op : sor<one<'='>, one<'>'>, one<'<'>, TAO_PEGTL_STRING("<="), TAO_PEGTL_STRING(">="), TAO_PEGTL_STRING("!=")> {};
    struct where_condition : if_must<pad<column_name, white_space>, where_op, pad<column_value, white_space>> {};
    struct where_logic : sor<where_and, where_or> {};
    struct where_group;
    struct where_group_priority : if_must<pad<one<'('>, white_space>, seq<star<white_space>, sor<where_group_priority, where_group>>, pad<one<')'>, white_space>> {};
    struct where_group : if_must<where_condition, star<seq<where_logic, sor<where_condition, where_group_priority>>>> {};
    struct where_start : TAO_PEGTL_ISTRING("where") {};
    struct where_stmt : if_must<where_start, plus<sor<where_group, where_group_priority>>> {};
    struct select_start : TAO_PEGTL_ISTRING("select") {};
    struct select_column : one<'*'> {};
    struct select_from : TAO_PEGTL_ISTRING("from") {};
    struct select_table_name : table_name {};
    struct select_stmt : if_must<select_start, pad<select_column, white_space>, select_from, plus<white_space>, select_table_name, star<white_space>, opt<where_stmt>, eof> {};

    template<typename Rule>
    using token_selector = parse_tree::selector<Rule,
                                                tao::pegtl::parse_tree::store_content::on<
                                                        select_stmt, integer,
                                                        select_table_name, column_value,
                                                        where_stmt, column_name, where_group_priority, where_logic, where_op, where_condition, where_group, floating>>;
    struct RestructureNode {
        void operator()(std::unique_ptr<parse_tree::node> &node, size_t depth = 0) {
            if (depth > 128)
                throw std::runtime_error("Maximum recursion depth");

            if (node->is_type<where_condition>()) {
                assert(node->children.size() == 3);
                node->children[1].swap(node->children[2]);
            }

            if (node->is_type<where_group>()) {
                for (auto i = 0; i < node->children.size(); i++) {
                    if (node->children[i]->is_type<where_logic>()) {
                        node->children[i].swap(node->children[i + 1]);
                        i++;
                    }
                }
            }

            for (auto &it: node->children) {
                this->operator()(it, depth + 1);
            }
        }
    };
}// namespace ToySQLEngine::SQLParser
