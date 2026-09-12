#include "ui/tui_state.hpp"

namespace ui {

const char* severity_label(Severity severity) {
    switch (severity) {
        case Severity::Info: return "INFO";
        case Severity::Warning: return "WARN";
        case Severity::Error: return "ERROR";
        case Severity::Success: return "OK";
    }
    return "INFO";
}

const char* state_label(const ApplicationSnapshot& snapshot) {
    if (snapshot.paused) return "PAUSED";
    if (snapshot.running) return "RUNNING";
    if (snapshot.errors > 0) return "ERROR";
    if (snapshot.ready) return "READY";
    return "STOPPED";
}

} // namespace ui
