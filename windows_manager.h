#pragma once

#include "imgui.h"
#include <memory>
#include <mutex>
#include <unordered_map>

namespace ToySQLEngine {
    template<class M>
    class WindowsLayer {
    public:
        explicit WindowsLayer(M &manager) : manager_(manager){};
        virtual ~WindowsLayer() = default;
        virtual void Draw() = 0;

    protected:
        M &manager_;
    };

    class WindowsManager {
    public:
        WindowsManager(size_t h, size_t w);
        ~WindowsManager() = default;

        static void SetFont(std::string_view font, float size) {
            auto &io = ImGui::GetIO();
            io.FontDefault = io.Fonts->AddFontFromFileTTF(font.data(), size, nullptr, io.Fonts->GetGlyphRangesChineseFull());
        }

        void Run();

        template<class T, class... Args>
        std::shared_ptr<T> CreateLayer(Args &&...args) {
            auto layer = std::make_shared<T>(*this, std::forward<Args>(args)...);
            std::shared_ptr<WindowsLayer<WindowsManager>> w_layer = std::static_pointer_cast<WindowsLayer<WindowsManager>>(layer);
            this->layer_maps_[(void *) layer.get()] = layer;
            return layer;
        };

    private:
        std::unordered_map<void *, std::shared_ptr<WindowsLayer<WindowsManager>>> layer_maps_;
    };
}// namespace ToySQLEngine