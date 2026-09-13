#pragma once

namespace ui {

enum class TuiColorRole {
    Border,
    Primary,
    Secondary,
    Muted,
    Accent,
    Success,
    Warning,
    Danger,
    Selected,
    Header
};

struct TuiTheme {
    short border{0};
    short primary{0};
    short secondary{0};
    short muted{0};
    short accent{0};
    short success{0};
    short warning{0};
    short danger{0};
    short selected{0};
    short header{0};

    // Logo pairs (brand white + red), used by draw_logo().
    short logo_light{0};
    short logo_red{0};
    short logo_light_on_red{0};
    short logo_red_on_light{0};

    bool colors_available{false};
    bool truecolor_available{false};
    bool unicode_available{false};

    void initialize();
    short pair(TuiColorRole role) const;
};

} // namespace ui
