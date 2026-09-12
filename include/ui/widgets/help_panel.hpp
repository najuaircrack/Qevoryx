#pragma once

#include "ui/widgets/panel.hpp"

namespace ui {

class HelpPanel final : public Panel {
public:
    void render(const TuiState& state,
                const ApplicationSnapshot& snapshot,
                const TuiTheme& theme) override;
};

} // namespace ui
