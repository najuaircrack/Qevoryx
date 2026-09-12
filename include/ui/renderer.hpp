#pragma once

#include "ui/tui_layout.hpp"
#include "ui/tui_state.hpp"
#include "ui/tui_theme.hpp"
#include "ui/widgets/actions_panel.hpp"
#include "ui/widgets/configuration_panel.hpp"
#include "ui/widgets/event_log_panel.hpp"
#include "ui/widgets/footer.hpp"
#include "ui/widgets/header.hpp"
#include "ui/widgets/help_panel.hpp"
#include "ui/widgets/live_status_panel.hpp"
#include "ui/widgets/modal.hpp"
#include "ui/widgets/runtime_panel.hpp"
#include "ui/widgets/status_panel.hpp"

#include <memory>

namespace ui {

class Renderer {
public:
    Renderer();
    ~Renderer();

    void initialize();
    void shutdown();

    void render(const TuiState& state,
                const TuiLayout& layout,
                const TuiTheme& theme,
                const ApplicationSnapshot& snapshot);

private:
    void render_main(const TuiState& state,
                     const TuiLayout& layout,
                     const TuiTheme& theme,
                     const ApplicationSnapshot& snapshot);
    void render_runtime(const TuiState& state,
                        const TuiLayout& layout,
                        const TuiTheme& theme,
                        const ApplicationSnapshot& snapshot);
    void render_help(const TuiState& state,
                     const TuiLayout& layout,
                     const TuiTheme& theme,
                     const ApplicationSnapshot& snapshot);
    void render_modal(const TuiState& state,
                      const TuiLayout& layout,
                      const TuiTheme& theme,
                      const ApplicationSnapshot& snapshot);

    HeaderWidget header_;
    ConfigurationPanel configuration_;
    ActionsPanel actions_;
    StatusPanel status_;
    EventLogPanel event_log_;
    FooterWidget footer_;
    HelpPanel help_;
    RuntimePanel runtime_;
    LiveStatusPanel live_status_;
    Modal modal_;
};

} // namespace ui
