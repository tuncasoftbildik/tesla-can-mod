#include "teslacan_app.h"
#include "teslacan_uart.h"

#include <stdio.h>
#include <string.h>

#define DASHBOARD_TICK_MS 500

static void submenu_cb(void* context, uint32_t index);
static bool nav_back_cb(void* context);
static void tick_cb(void* context);

static void rebuild_dashboard(TeslaCanApp* app) {
    char l1[48], l2[48], l3[48], l4[48], l5[48], header[24];

    furi_mutex_acquire(app->state_mutex, FuriWaitForever);
    bool stale = (furi_get_tick() - app->last_event_tick) > furi_ms_to_ticks(2500);

    snprintf(header, sizeof(header), "TeslaCAN %s %s",
             app->hw_tag[0] ? app->hw_tag : "?",
             stale ? "(no link)" : "");
    snprintf(l1, sizeof(l1), "FSD: %s   Mode: %d   CAN: %s",
             app->fsd_enabled ? "ON" : "off",
             app->speed_mode,
             app->can_running ? "run" : "stop");
    snprintf(l2, sizeof(l2), "SoC %.1f%%   %.0f V",
             (double)app->soc_percent, (double)app->pack_voltage);
    snprintf(l3, sizeof(l3), "Power: %.2f kW", (double)app->pack_power_kw);
    snprintf(l4, sizeof(l4), "Batt T: %d..%d C   Wh/km: %.0f",
             app->pack_tmin, app->pack_tmax, (double)app->wh_per_km);
    snprintf(l5, sizeof(l5), "Frames: %lu   Precond: %s",
             (unsigned long)app->frames_rx,
             app->precond_active ? "ON" : "off");
    furi_mutex_release(app->state_mutex);

    widget_reset(app->widget_dashboard);
    widget_add_string_element(app->widget_dashboard, 0,  2,  AlignLeft, AlignTop, FontPrimary,   header);
    widget_add_string_element(app->widget_dashboard, 0,  16, AlignLeft, AlignTop, FontSecondary, l1);
    widget_add_string_element(app->widget_dashboard, 0,  26, AlignLeft, AlignTop, FontSecondary, l2);
    widget_add_string_element(app->widget_dashboard, 0,  36, AlignLeft, AlignTop, FontSecondary, l3);
    widget_add_string_element(app->widget_dashboard, 0,  46, AlignLeft, AlignTop, FontSecondary, l4);
    widget_add_string_element(app->widget_dashboard, 0,  56, AlignLeft, AlignTop, FontSecondary, l5);
}

static void tick_cb(void* context) {
    TeslaCanApp* app = context;
    uint32_t current = view_dispatcher_get_current_view(app->view_dispatcher);
    if(current == TeslaCanViewDashboard) {
        rebuild_dashboard(app);
    }
}

static void submenu_cb(void* context, uint32_t index) {
    TeslaCanApp* app = context;
    switch(index) {
        case TeslaCanMenuDashboard:
            rebuild_dashboard(app);
            view_dispatcher_switch_to_view(app->view_dispatcher, TeslaCanViewDashboard);
            break;
        case TeslaCanMenuToggleFSD: {
            furi_mutex_acquire(app->state_mutex, FuriWaitForever);
            bool next = !app->fsd_enabled;
            furi_mutex_release(app->state_mutex);
            teslacan_uart_send_cmd(next ? "CMD FSD on" : "CMD FSD off");
            break;
        }
        case TeslaCanMenuTogglePrecond: {
            furi_mutex_acquire(app->state_mutex, FuriWaitForever);
            bool next = !app->precond_active;
            furi_mutex_release(app->state_mutex);
            teslacan_uart_send_cmd(next ? "CMD PRECOND on" : "CMD PRECOND off");
            break;
        }
        case TeslaCanMenuCycleMode: {
            furi_mutex_acquire(app->state_mutex, FuriWaitForever);
            int next = (app->speed_mode + 1) % 5;
            furi_mutex_release(app->state_mutex);
            char buf[16];
            snprintf(buf, sizeof(buf), "CMD MODE %d", next);
            teslacan_uart_send_cmd(buf);
            break;
        }
        case TeslaCanMenuAbout:
            widget_reset(app->widget_dashboard);
            widget_add_string_element(app->widget_dashboard, 0,  2,  AlignLeft, AlignTop, FontPrimary,   "TeslaCAN");
            widget_add_string_element(app->widget_dashboard, 0,  16, AlignLeft, AlignTop, FontSecondary, "Flipper companion");
            widget_add_string_element(app->widget_dashboard, 0,  26, AlignLeft, AlignTop, FontSecondary, "for ESP32-C6 firmware");
            widget_add_string_element(app->widget_dashboard, 0,  40, AlignLeft, AlignTop, FontSecondary, "github.com/");
            widget_add_string_element(app->widget_dashboard, 0,  50, AlignLeft, AlignTop, FontSecondary, "tuncasoftbildik/tesla-can-mod");
            view_dispatcher_switch_to_view(app->view_dispatcher, TeslaCanViewDashboard);
            break;
        default:
            break;
    }
}

static bool nav_back_cb(void* context) {
    TeslaCanApp* app = context;
    uint32_t current = view_dispatcher_get_current_view(app->view_dispatcher);
    if(current == TeslaCanViewDashboard) {
        view_dispatcher_switch_to_view(app->view_dispatcher, TeslaCanViewSubmenu);
        return true;
    }
    view_dispatcher_stop(app->view_dispatcher);
    return true;
}

static TeslaCanApp* app_alloc(void) {
    TeslaCanApp* app = malloc(sizeof(TeslaCanApp));
    memset(app, 0, sizeof(*app));

    app->state_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->view_dispatcher, nav_back_cb);
    view_dispatcher_set_tick_event_callback(app->view_dispatcher, tick_cb, DASHBOARD_TICK_MS);
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    app->submenu = submenu_alloc();
    submenu_set_header(app->submenu, "TeslaCAN");
    submenu_add_item(app->submenu, "Dashboard",      TeslaCanMenuDashboard,     submenu_cb, app);
    submenu_add_item(app->submenu, "Toggle FSD",     TeslaCanMenuToggleFSD,     submenu_cb, app);
    submenu_add_item(app->submenu, "Toggle Precond", TeslaCanMenuTogglePrecond, submenu_cb, app);
    submenu_add_item(app->submenu, "Cycle Mode",     TeslaCanMenuCycleMode,     submenu_cb, app);
    submenu_add_item(app->submenu, "About",          TeslaCanMenuAbout,         submenu_cb, app);
    view_dispatcher_add_view(app->view_dispatcher, TeslaCanViewSubmenu, submenu_get_view(app->submenu));

    app->widget_dashboard = widget_alloc();
    view_dispatcher_add_view(app->view_dispatcher, TeslaCanViewDashboard, widget_get_view(app->widget_dashboard));

    return app;
}

static void app_free(TeslaCanApp* app) {
    view_dispatcher_remove_view(app->view_dispatcher, TeslaCanViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, TeslaCanViewDashboard);
    submenu_free(app->submenu);
    widget_free(app->widget_dashboard);
    view_dispatcher_free(app->view_dispatcher);
    furi_record_close(RECORD_GUI);
    furi_mutex_free(app->state_mutex);
    free(app);
}

int32_t teslacan_app_main(void* p) {
    UNUSED(p);
    TeslaCanApp* app = app_alloc();
    teslacan_uart_start(app);

    view_dispatcher_switch_to_view(app->view_dispatcher, TeslaCanViewSubmenu);
    view_dispatcher_run(app->view_dispatcher);

    teslacan_uart_stop(app);
    app_free(app);
    return 0;
}
