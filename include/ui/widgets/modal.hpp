#pragma once

#include "ui/widgets/panel.hpp"

#include <string>

namespace ui {

class Modal final : public Panel {
public:
    void set_content(const std::string& title,
                     const std::vector<std::string>& lines,
                     const std::string& cancel_label,
                     const std::string& confirm_label);

    void render(const TuiState& state,
                const ApplicationSnapshot& snapshot,
                const TuiTheme& theme) override;

private:
    std::string title_;
    std::vector<std::string> lines_;
    std::string cancel_label_{"Cancel"};
    std::string confirm_label_{"Confirm"};
};

} // namespace ui
