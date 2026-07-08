#pragma once

#include <string>
#include <functional>
#include <queue>
#include <optional>

namespace ml::ui {
    class PopupManager
    {
    public:
        static PopupManager& getInstance();

        void errorPopup(std::string text, std::function<void()> onOk = nullptr);
        void draw();

    private:
        PopupManager() = default;
        ~PopupManager() = default;
        PopupManager(const PopupManager&) = delete;
        PopupManager& operator=(const PopupManager&) = delete;
        PopupManager(PopupManager&&) noexcept = delete;
        PopupManager& operator=(PopupManager&&) noexcept = delete;

        struct PopupEntry
        {
            std::string text;
            std::function<void()> onOk = nullptr;
        };

        std::queue<PopupEntry> pending_;
        std::optional<PopupEntry> current_;
    };
}