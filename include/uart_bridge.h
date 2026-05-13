#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#include "handlers.h"
#include "drivers/twai_driver.h"
#include "log_buffer.h"

// ─── Flipper Zero UART Bridge ─────────────────────────────────────────────
//
// Exposes the TeslaCAN state over UART1 so a Flipper Zero (or any host with
// a 3.3V UART) can read live telemetry and send control commands.
//
// Wiring (Flipper Zero GPIO ↔ ESP32-C6):
//   Flipper GND  (pin 8 / 18)  ↔ ESP32-C6 GND
//   Flipper 3V3  (pin 9)       ↔ ESP32-C6 3V3   (only if Flipper is unpowered)
//   Flipper TX   (pin 13)      ↔ ESP32-C6 RX    (FLIPPER_UART_RX, default GPIO 5)
//   Flipper RX   (pin 14)      ↔ ESP32-C6 TX    (FLIPPER_UART_TX, default GPIO 4)
//
// Wire protocol — line-based ASCII, '\n' terminated, 115200 8N1.
//
// Events (ESP32 → Flipper, periodic at 2 Hz when subscribed):
//   EVT HELLO ver=<x.y.z> hw=<HW3|HW4|LEGACY>
//   EVT STATUS fsd=<0|1> mode=<0..4> uptime=<ms> frames=<n> sent=<n>
//   EVT BATTERY soc=<%.1f> v=<%.2f> i=<%.2f> kw=<%.2f> tmin=<%d> tmax=<%d> wh=<%.1f>
//   EVT CAN state=<run|stop|err> rx=<n> sent=<n>
//   EVT PRECOND active=<0|1> allowed=<0|1> worth=<0|1>
//   EVT LOG <free-form message>
//
// Commands (Flipper → ESP32):
//   CMD HELLO                        → ACK HELLO
//   CMD STATUS                       → ACK STATUS, then a STATUS+BATTERY snapshot
//   CMD FSD <on|off>                 → ACK FSD <state> | ERR FSD <reason>
//   CMD MODE <0..4>                  → ACK MODE <n>    | ERR MODE range
//   CMD PRECOND <on|off>             → ACK PRECOND <state>
//   CMD ISA <on|off>                 → ACK ISA <state>
//   CMD LOG <on|off>                 → ACK LOG <state>   (toggles EVT LOG mirroring)
//   CMD STREAM <on|off>              → ACK STREAM <state> (toggles periodic EVTs)
//
// Notes:
// - The bridge is non-blocking; uart_bridge_loop() runs alongside CAN and web.
// - Setting STREAM=off (default) keeps the UART quiet — Flipper requests
//   snapshots on demand. This avoids saturating the link when idle.
// - This bridge READS state from the active handler and writes ONLY to the
//   runtime toggle fields; it does not send CAN frames directly. The handler
//   is still the only writer to the bus.

#ifndef FLIPPER_UART_TX
#define FLIPPER_UART_TX 4
#endif
#ifndef FLIPPER_UART_RX
#define FLIPPER_UART_RX 5
#endif
#ifndef FLIPPER_UART_BAUD
#define FLIPPER_UART_BAUD 115200
#endif
#ifndef FLIPPER_FW_VERSION
#define FLIPPER_FW_VERSION "0.2.0"
#endif

class UartBridge
{
public:
    void begin(CarManagerBase *handler, TWAIDriver *can)
    {
        handler_ = handler;
        can_ = can;
        port_.begin(FLIPPER_UART_BAUD, SERIAL_8N1, FLIPPER_UART_RX, FLIPPER_UART_TX);
        emitHello();
    }

    void loop()
    {
        // Drain incoming bytes into rxBuf_, dispatch on newline.
        while (port_.available())
        {
            char c = (char)port_.read();
            if (c == '\r') continue;
            if (c == '\n')
            {
                rxBuf_[rxLen_] = 0;
                if (rxLen_ > 0) dispatch(rxBuf_);
                rxLen_ = 0;
                continue;
            }
            if (rxLen_ < sizeof(rxBuf_) - 1)
            {
                rxBuf_[rxLen_++] = c;
            }
            else
            {
                rxLen_ = 0; // overflow — drop frame
            }
        }

        // Periodic event emission (500 ms cadence) when streaming enabled.
        if (streamEnabled_)
        {
            unsigned long now = millis();
            if (now - lastEmit_ >= 500)
            {
                lastEmit_ = now;
                emitStatus();
                emitBattery();
                emitCanStats();
                emitPrecond();
            }
        }

        pollLog();
    }

    bool isLogMirroring() const { return logMirror_; }

private:
    // Poll globalLog for new entries and mirror them when logMirror_ is on.
    void pollLog()
    {
        if (!logMirror_) { lastLogCount_ = globalLog.count(); return; }
        int now = globalLog.count();
        if (now == lastLogCount_) return;
        // Emit only the delta. globalLog is a circular buffer indexed 0..count-1
        // where count saturates at MAX_ENTRIES; we just dump the tail.
        int from = lastLogCount_;
        if (now < from) from = 0; // count rolled over
        for (int i = from; i < now; i++)
        {
            port_.printf("EVT LOG %s\n", globalLog.get(i).message);
        }
        lastLogCount_ = now;
    }

    HardwareSerial port_{1};   // UART1 (UART0 is USB-CDC)
    CarManagerBase *handler_ = nullptr;
    TWAIDriver *can_ = nullptr;

    char rxBuf_[96];
    size_t rxLen_ = 0;
    unsigned long lastEmit_ = 0;
    int lastLogCount_ = 0;
    bool streamEnabled_ = false;
    bool logMirror_ = false;

    static bool parseOnOff(const char *arg, bool &out)
    {
        if (!arg) return false;
        if (!strcasecmp(arg, "on") || !strcmp(arg, "1"))  { out = true;  return true; }
        if (!strcasecmp(arg, "off") || !strcmp(arg, "0")) { out = false; return true; }
        return false;
    }

    void dispatch(char *line)
    {
        char *cmd = strtok(line, " ");
        if (!cmd) return;
        if (strcasecmp(cmd, "CMD")) return; // ignore non-CMD lines

        char *verb = strtok(nullptr, " ");
        if (!verb) return;
        char *arg  = strtok(nullptr, " ");

        if (!strcasecmp(verb, "HELLO"))
        {
            ack("HELLO");
            emitHello();
        }
        else if (!strcasecmp(verb, "STATUS"))
        {
            ack("STATUS");
            emitStatus();
            emitBattery();
        }
        else if (!strcasecmp(verb, "FSD"))
        {
            bool v; if (!parseOnOff(arg, v)) { err("FSD", "arg"); return; }
            if (handler_) handler_->fsdEnabled = v;
            port_.printf("ACK FSD %s\n", v ? "on" : "off");
        }
        else if (!strcasecmp(verb, "MODE"))
        {
            if (!arg) { err("MODE", "arg"); return; }
            int n = atoi(arg);
            if (n < 0 || n > 4) { err("MODE", "range"); return; }
            if (handler_) handler_->speedProfile = n;
            port_.printf("ACK MODE %d\n", n);
        }
        else if (!strcasecmp(verb, "PRECOND"))
        {
            bool v; if (!parseOnOff(arg, v)) { err("PRECOND", "arg"); return; }
            if (handler_) handler_->precondRequested = v;
            port_.printf("ACK PRECOND %s\n", v ? "on" : "off");
        }
        else if (!strcasecmp(verb, "ISA"))
        {
            bool v; if (!parseOnOff(arg, v)) { err("ISA", "arg"); return; }
            if (handler_) handler_->isaSuppress = v;
            port_.printf("ACK ISA %s\n", v ? "on" : "off");
        }
        else if (!strcasecmp(verb, "LOG"))
        {
            bool v; if (!parseOnOff(arg, v)) { err("LOG", "arg"); return; }
            logMirror_ = v;
            port_.printf("ACK LOG %s\n", v ? "on" : "off");
        }
        else if (!strcasecmp(verb, "STREAM"))
        {
            bool v; if (!parseOnOff(arg, v)) { err("STREAM", "arg"); return; }
            streamEnabled_ = v;
            port_.printf("ACK STREAM %s\n", v ? "on" : "off");
        }
        else
        {
            err(verb, "unknown");
        }
    }

    void ack(const char *what)       { port_.printf("ACK %s\n", what); }
    void err(const char *what, const char *why) { port_.printf("ERR %s %s\n", what, why); }

    const char *hwTag() const
    {
#if defined(HW4)
        return "HW4";
#elif defined(HW3)
        return "HW3";
#elif defined(LEGACY)
        return "LEGACY";
#else
        return "UNKNOWN";
#endif
    }

    void emitHello()
    {
        port_.printf("EVT HELLO ver=%s hw=%s\n", FLIPPER_FW_VERSION, hwTag());
    }

    void emitStatus()
    {
        if (!handler_) return;
        port_.printf("EVT STATUS fsd=%d mode=%d uptime=%lu frames=%lu sent=%lu\n",
                     handler_->fsdEnabled ? 1 : 0,
                     handler_->speedProfile,
                     (unsigned long)millis(),
                     (unsigned long)handler_->frameCount,
                     (unsigned long)handler_->sentCount);
    }

    void emitBattery()
    {
        if (!handler_) return;
        port_.printf("EVT BATTERY soc=%.1f v=%.2f i=%.2f kw=%.2f tmin=%d tmax=%d wh=%.1f\n",
                     handler_->socPercent,
                     handler_->packVoltage,
                     handler_->packCurrent,
                     handler_->packPowerKW,
                     (int)handler_->packTempMin,
                     (int)handler_->packTempMax,
                     handler_->whPerKm);
    }

    void emitCanStats()
    {
        if (!handler_ || !can_) return;
        auto diag = can_->getDiagnostics();
        port_.printf("EVT CAN state=%s rx=%lu sent=%lu rxerr=%lu txerr=%lu\n",
                     diag.state,
                     (unsigned long)handler_->frameCount,
                     (unsigned long)handler_->sentCount,
                     (unsigned long)diag.rxErrors,
                     (unsigned long)diag.txErrors);
    }

    void emitPrecond()
    {
        if (!handler_) return;
        port_.printf("EVT PRECOND active=%d allowed=%d worth=%d\n",
                     handler_->precondActive ? 1 : 0,
                     handler_->precondAllowed ? 1 : 0,
                     handler_->precondWorthwhile ? 1 : 0);
    }
};

extern UartBridge uartBridge;
