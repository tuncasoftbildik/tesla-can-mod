#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/widget.h>

#define TESLACAN_TEXT_BUF 96

typedef struct {
    // GUI plumbing
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    Submenu* submenu;
    Widget* widget_dashboard;

    // UART worker
    volatile bool worker_running;

    // Latest parsed state (mutated by UART thread, read by GUI thread)
    FuriMutex* state_mutex;
    bool fsd_enabled;
    int speed_mode;
    float soc_percent;
    float pack_voltage;
    float pack_power_kw;
    int pack_tmin;
    int pack_tmax;
    float wh_per_km;
    bool precond_active;
    bool can_running;
    uint32_t frames_rx;
    char hw_tag[8];
    char fw_version[16];
    uint32_t last_event_tick;
} TeslaCanApp;

typedef enum {
    TeslaCanViewSubmenu,
    TeslaCanViewDashboard,
} TeslaCanView;

typedef enum {
    TeslaCanMenuDashboard,
    TeslaCanMenuToggleFSD,
    TeslaCanMenuTogglePrecond,
    TeslaCanMenuCycleMode,
    TeslaCanMenuAbout,
} TeslaCanMenu;
