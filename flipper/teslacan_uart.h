#pragma once

#include "teslacan_app.h"

// UART line parser worker. Reads ASCII lines from USART1 (Flipper GPIO 13/14
// = pin TX/RX) at 115200 baud and decodes EVT messages from the ESP32-C6.
// Pushes parsed updates into the app state under state_mutex.
//
// Outgoing CMD lines are written via teslacan_uart_send_cmd() from the GUI
// thread; the worker thread is read-only on the UART RX side.

void teslacan_uart_start(TeslaCanApp* app);
void teslacan_uart_stop(TeslaCanApp* app);
void teslacan_uart_send_cmd(const char* line);
