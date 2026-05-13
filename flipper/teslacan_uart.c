#include "teslacan_uart.h"

#include <furi.h>
#include <furi_hal_serial.h>
#include <furi_hal_serial_control.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UART_BAUD       115200
#define LINE_BUF_LEN    128
#define STREAM_BUF_LEN  512

static FuriHalSerialHandle* s_uart_handle = NULL;
static FuriStreamBuffer*    s_rx_stream   = NULL;
static FuriThread*          s_thread      = NULL;
static TeslaCanApp*         s_app         = NULL;

static void rx_event_cb(FuriHalSerialHandle* handle, FuriHalSerialRxEvent event, void* ctx) {
    UNUSED(ctx);
    if(event == FuriHalSerialRxEventData) {
        uint8_t byte = furi_hal_serial_async_rx(handle);
        if(s_rx_stream) {
            furi_stream_buffer_send(s_rx_stream, &byte, 1, 0);
        }
    }
}

static int extract_kv_float(const char* s, const char* key, float* out) {
    const char* p = strstr(s, key);
    if(!p) return 0;
    p += strlen(key);
    *out = (float)atof(p);
    return 1;
}

static int extract_kv_int(const char* s, const char* key, int* out) {
    const char* p = strstr(s, key);
    if(!p) return 0;
    p += strlen(key);
    *out = atoi(p);
    return 1;
}

static void parse_line(TeslaCanApp* app, const char* line) {
    if(strncmp(line, "EVT ", 4) != 0) return;
    const char* tag = line + 4;

    furi_mutex_acquire(app->state_mutex, FuriWaitForever);
    app->last_event_tick = furi_get_tick();

    if(strncmp(tag, "STATUS ", 7) == 0) {
        int v;
        if(extract_kv_int(tag, "fsd=", &v))     app->fsd_enabled = (v != 0);
        if(extract_kv_int(tag, "mode=", &v))    app->speed_mode  = v;
        unsigned long u;
        const char* p = strstr(tag, "frames=");
        if(p) { u = strtoul(p + 7, NULL, 10); app->frames_rx = (uint32_t)u; }
    }
    else if(strncmp(tag, "BATTERY ", 8) == 0) {
        extract_kv_float(tag, "soc=", &app->soc_percent);
        extract_kv_float(tag, "v=",   &app->pack_voltage);
        extract_kv_float(tag, "kw=",  &app->pack_power_kw);
        extract_kv_float(tag, "wh=",  &app->wh_per_km);
        extract_kv_int  (tag, "tmin=",&app->pack_tmin);
        extract_kv_int  (tag, "tmax=",&app->pack_tmax);
    }
    else if(strncmp(tag, "CAN ", 4) == 0) {
        app->can_running = (strstr(tag, "state=run") != NULL) ||
                           (strstr(tag, "state=RUNNING") != NULL);
    }
    else if(strncmp(tag, "PRECOND ", 8) == 0) {
        int v;
        if(extract_kv_int(tag, "active=", &v)) app->precond_active = (v != 0);
    }
    else if(strncmp(tag, "HELLO ", 6) == 0) {
        const char* p = strstr(tag, "hw=");
        if(p) {
            strncpy(app->hw_tag, p + 3, sizeof(app->hw_tag) - 1);
            // strip trailing whitespace/newline
            for(size_t i = 0; i < sizeof(app->hw_tag); i++) {
                if(app->hw_tag[i] == ' ' || app->hw_tag[i] == '\n' || app->hw_tag[i] == '\r') {
                    app->hw_tag[i] = 0;
                    break;
                }
            }
        }
        p = strstr(tag, "ver=");
        if(p) {
            strncpy(app->fw_version, p + 4, sizeof(app->fw_version) - 1);
            for(size_t i = 0; i < sizeof(app->fw_version); i++) {
                if(app->fw_version[i] == ' ' || app->fw_version[i] == '\n' || app->fw_version[i] == '\r') {
                    app->fw_version[i] = 0;
                    break;
                }
            }
        }
    }

    furi_mutex_release(app->state_mutex);
}

static int32_t uart_worker(void* ctx) {
    TeslaCanApp* app = (TeslaCanApp*)ctx;
    char line[LINE_BUF_LEN];
    size_t len = 0;

    while(app->worker_running) {
        uint8_t byte;
        if(furi_stream_buffer_receive(s_rx_stream, &byte, 1, 100) == 1) {
            if(byte == '\r') continue;
            if(byte == '\n' || len >= sizeof(line) - 1) {
                line[len] = 0;
                if(len > 0) parse_line(app, line);
                len = 0;
            } else {
                line[len++] = (char)byte;
            }
        }
    }
    return 0;
}

void teslacan_uart_start(TeslaCanApp* app) {
    s_app = app;

    s_rx_stream = furi_stream_buffer_alloc(STREAM_BUF_LEN, 1);

    s_uart_handle = furi_hal_serial_control_acquire(FuriHalSerialIdUsart);
    furi_check(s_uart_handle != NULL);
    furi_hal_serial_init(s_uart_handle, UART_BAUD);
    furi_hal_serial_async_rx_start(s_uart_handle, rx_event_cb, NULL, false);

    app->worker_running = true;
    s_thread = furi_thread_alloc_ex("TeslaCAN-UART", 2048, uart_worker, app);
    furi_thread_start(s_thread);

    // Handshake
    teslacan_uart_send_cmd("CMD HELLO");
    teslacan_uart_send_cmd("CMD STREAM on");
}

void teslacan_uart_stop(TeslaCanApp* app) {
    if(app->worker_running) {
        teslacan_uart_send_cmd("CMD STREAM off");
        app->worker_running = false;
    }
    if(s_thread) {
        furi_thread_join(s_thread);
        furi_thread_free(s_thread);
        s_thread = NULL;
    }
    if(s_uart_handle) {
        furi_hal_serial_async_rx_stop(s_uart_handle);
        furi_hal_serial_deinit(s_uart_handle);
        furi_hal_serial_control_release(s_uart_handle);
        s_uart_handle = NULL;
    }
    if(s_rx_stream) {
        furi_stream_buffer_free(s_rx_stream);
        s_rx_stream = NULL;
    }
    s_app = NULL;
}

void teslacan_uart_send_cmd(const char* line) {
    if(!s_uart_handle || !line) return;
    size_t n = strlen(line);
    furi_hal_serial_tx(s_uart_handle, (const uint8_t*)line, n);
    furi_hal_serial_tx(s_uart_handle, (const uint8_t*)"\n", 1);
}
