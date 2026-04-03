#pragma once

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include "web_ui.h"
#include "../can_helpers.h"
#include "../log_buffer.h"
#include "../handlers.h"
#include "../drivers/twai_driver.h"

static WebServer server(80);
static DNSServer dnsServer;
static Preferences prefs;
static Shared<bool> enablePrint{true};
static CarManagerBase *webHandler = nullptr;
static TWAIDriver *webTwaiDriver = nullptr;

static void webServerInit()
{
    prefs.begin("teslaCAN", false);
    bool savedForce = prefs.getBool("forceFSD", false);
    forceFSDRuntime = savedForce;

    WiFi.mode(WIFI_AP);
    WiFi.softAP("TeslaCAN", "tesla1234");
    delay(100);

    dnsServer.start(53, "*", WiFi.softAPIP());

    server.on("/", HTTP_GET, []() {
        server.send(200, "text/html", WEB_UI_HTML);
    });

    server.on("/api/status", HTTP_GET, []() {
        int prof = webHandler ? webHandler->speedProfile : 0;
        int soff = webHandler ? webHandler->speedOffset : 0;
        bool fsd = webHandler ? webHandler->fsdEnabled : false;
        uint32_t rx = webHandler ? webHandler->frameCount : 0;
        uint32_t tx = webHandler ? webHandler->sentCount : 0;

        const char *canState = "UNKNOWN";
        uint32_t rxErr = 0, txErr = 0, busErr = 0, rxMiss = 0;
        if (webTwaiDriver)
        {
            auto diag = webTwaiDriver->getDiagnostics();
            canState = diag.state;
            rxErr = diag.rxErrors;
            txErr = diag.txErrors;
            busErr = diag.busErrors;
            rxMiss = diag.rxMissed;
        }

        float bvolt = webHandler ? webHandler->packVoltage : 0;
        float bcurr = webHandler ? webHandler->packCurrent : 0;
        float bpow  = webHandler ? webHandler->packPowerKW : 0;
        float bsoc  = webHandler ? webHandler->socPercent : 0;
        float btmin = webHandler ? webHandler->packTempMin : 0;
        float btmax = webHandler ? webHandler->packTempMax : 0;
        float bwhkm = webHandler ? webHandler->whPerKm : 0;
        bool precond = webHandler ? webHandler->precondActive : false;
        bool precondReq = webHandler ? webHandler->precondRequested : false;

        char json[768];
        snprintf(json, sizeof(json),
            "{\"fsd_enabled\":%s,\"force_fsd\":%s,\"speed_profile\":%d,"
            "\"speed_offset\":%d,\"uptime_s\":%lu,\"enable_print\":%s,"
            "\"can\":{\"state\":\"%s\",\"frames_received\":%lu,\"frames_sent\":%lu,"
            "\"rx_errors\":%lu,\"tx_errors\":%lu,\"bus_errors\":%lu,\"rx_missed\":%lu},"
            "\"bat\":{\"voltage\":%.1f,\"current\":%.1f,\"power_kw\":%.2f,"
            "\"soc\":%.1f,\"temp_min\":%.1f,\"temp_max\":%.1f,"
            "\"wh_per_km\":%.0f,\"precond\":%s,\"precond_req\":%s},"
            "\"log_head\":%d,\"logs\":[",
            fsd ? "true" : "false",
            (bool)forceFSDRuntime ? "true" : "false",
            prof, soff,
            (unsigned long)(millis() / 1000),
            (bool)enablePrint ? "true" : "false",
            canState,
            (unsigned long)rx, (unsigned long)tx,
            (unsigned long)rxErr, (unsigned long)txErr,
            (unsigned long)busErr, (unsigned long)rxMiss,
            bvolt, bcurr, bpow, bsoc, btmin, btmax, bwhkm,
            precond ? "true" : "false",
            precondReq ? "true" : "false",
            globalLog.head());

        String response = json;

        // Append recent logs
        int count = globalLog.count();
        int start = 0;
        if (server.hasArg("log_since"))
        {
            // Only send new logs
            start = count > 20 ? count - 20 : 0;
        }
        for (int i = start; i < count; i++)
        {
            const auto &entry = globalLog.get(i);
            if (i > start)
                response += ",";
            response += "{\"ts\":";
            response += entry.timestamp;
            response += ",\"msg\":\"";
            response += entry.message;
            response += "\"}";
        }
        response += "]}";

        server.send(200, "application/json", response);
    });

    server.on("/api/force-fsd", HTTP_POST, []() {
        if (server.hasArg("plain"))
        {
            String body = server.arg("plain");
            bool enable = body.indexOf("true") >= 0;
            forceFSDRuntime = enable;
            prefs.putBool("forceFSD", enable);
            globalLog.add(enable ? "Force FSD: ON" : "Force FSD: OFF");
        }
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/api/precond", HTTP_POST, []() {
        if (server.hasArg("plain") && webHandler)
        {
            String body = server.arg("plain");
            bool enable = body.indexOf("true") >= 0;
            webHandler->precondRequested = enable;
            if (!enable)
                webHandler->precondActive = false;
            globalLog.add(enable ? "Precondition: ON" : "Precondition: OFF");
        }
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/api/enable-print", HTTP_POST, []() {
        if (server.hasArg("plain"))
        {
            String body = server.arg("plain");
            enablePrint = body.indexOf("true") >= 0;
        }
        server.send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.onNotFound([]() {
        server.sendHeader("Location", "/", true);
        server.send(302, "text/plain", "");
    });

    server.begin();
    globalLog.add("Web server started");
    Serial.println("WiFi AP: TeslaCAN (192.168.4.1)");
}

static void webServerLoop()
{
    dnsServer.processNextRequest();
    server.handleClient();
}
