/**
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 *
 * This file provides the main entry point for the AMP Server. All of the 
 * major components are instantiated and hooked together in this file so
 * it should be a good place to start to navigate the rest of the application.
 */
#include <execinfo.h>
#include <signal.h>
#include <iostream>

// 3rd party HTTP/HTTPS client
#include <curl/curl.h>
// 3rd party command-line parser
#include <argparse/argparse.hpp>

// Non-AMP stuff from my C++ tools library
#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/linux/StdClock.h"
#include "kc1fsz-tools/linux/MTLog.h"

// All of the AMP stuff
#include "TraceLog.h"
#include "LineIAX2.h"
#include "LineUsb.h"
#include "EventLoop.h"
#include "Bridge.h"
#include "BridgeCall.h"
#include "MultiRouter.h"
#include "WebUi.h"
#include "ConfigPoller.h"
#include "SignalIn.h"
#include "ThreadUtil.h"
#include "service-thread.h"
#include "CallValidatorStd.h"
#include "LocalRegistryStd.h"
#include "config-handler.h"

using namespace std;
using namespace kc1fsz;

// ### TODO: FIGURE OUT HOW TO MAKE THIS AUTOMATIC
static const char* VERSION = "20260114.0";
const char* const GIT_HASH = "?";

static void sigHandler(int sig);

int main(int argc, const char** argv) {

    // Name the thread
    amp::setThreadName("amp-server");
    // Install the crash stack handler
    signal(SIGSEGV, sigHandler);

    MTLog log;
    log.info("AMP Server");
    log.info("Powered by the Ampersand ASL Project https://github.com/Ampersand-ASL");
    log.info("Copyright (C) 2026, Bruce MacKinnon KC1FSZ");
    log.info("Version %s Git Hash %s", VERSION, GIT_HASH);
    log.info("----------------------------------------------------------------------");

    StdClock clock;

    // A special log used for tracing/performance analysis
    const unsigned traceLogDataLen = 1024;
    std::string traceLogData[traceLogDataLen];
    TraceLog traceLog(clock, traceLogData, traceLogDataLen);

    // Get libcurl going
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res) {
        log.error("Libcurl failed to initialize %d", res);
        std::exit(1);
    }

    // Parse command line arguments
    argparse::ArgumentParser program("amp-server", VERSION);
    string cfgFileName;
    string defaultCfgFileName = getenv("HOME");
    defaultCfgFileName += "/amp-server.json";
    program.add_argument("--config")
        .help("Name of configuration file")
        .default_value(defaultCfgFileName)
        .store_into(cfgFileName);
    
    int uiPort = 8080;
    program.add_argument("--httpport")
        .store_into(uiPort)
        .default_value(8080)
        .help("Port number for HTTP UI server");

    program.add_argument("--trace")
        .help("Turn on network tracing")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        log.error("Argument error: %s", err.what());
        std::exit(1);
    }

    log.info("Using configuration file %s", cfgFileName.c_str());

    if (!filesystem::exists(cfgFileName)) {
        log.info("Creating default configuration");
        ofstream cfg(cfgFileName);
        if (cfg.is_open()) 
            cfg << amp::ConfigPoller::DEFAULT_CONFIG << endl;
        else {
            log.error("Unable to create default configuration");
            std::exit(1);
        }
    }

    // Get the service thread running. This handles non-time-sensitive
    // stuff like registration, stats, etc.
    service_thread_args args1;
    args1.log = &log;
    args1.cfgFileName = cfgFileName;
    std::thread serviceThread(service_thread, &args1);

    // This is the "bus" that passes Message objects between the rest of the 
    // components in ths system
    MultiRouter router;

    // The Bridge is what provides the conference capability
    amp::Bridge bridge10(log, traceLog, clock, router, amp::BridgeCall::Mode::NORMAL);
    router.addRoute(&bridge10, 10);

    // This is the connection to the USB sound interface
    LineUsb radio2(log, clock, router, 2, 1, 10, 1);
    router.addRoute(&radio2, 2);

    // This manages the COS signal detect
    amp::SignalIn signalIn3(log, clock, router, 2, 
        Message::SignalType::COS_ON, Message::SignalType::COS_OFF);
    router.addRoute(&signalIn3, 3);

    // This is the IAX2 network connection
    CallValidatorStd val;
    LocalRegistryStd locReg;
    LineIAX2 iax2Channel1(log, traceLog, clock, 1, router, &val, &locReg, 10);
    router.addRoute(&iax2Channel1, 1);
    if (program["--trace"] == true)
        iax2Channel1.setTrace(true);

    // Instantiate the server for the HTTP-based UI
    amp::WebUi webUi(log, clock, router, uiPort, 1, 2, cfgFileName.c_str(), VERSION,
        traceLog);
    // This allow the WebUi to watch all traffic and pull out the things 
    // that are relevant for status display.
    router.addRoute(&webUi, MultiRouter::BROADCAST);

    // Setup the configuration poller for this thread 
    amp::ConfigPoller cfgPoller(log, cfgFileName.c_str(), 
        // This function will be called on any update to the configuration document.
        [&log, &webUi, &iax2Channel1, &radio2, &signalIn3, &bridge10]
        (const json& cfg) {

            log.info("Configuration change detected");
            cout << cfg.dump() << endl;

            try {
                amp::configHandler(log, cfg, webUi, iax2Channel1, radio2, signalIn3, bridge10);
            }
            // ### TODO MORE SPECIFIC
            catch (json::exception& ex) {
                log.error("Failed to process configuration change %s", ex.what());
            }
        }
    );

    // Main loop        
    Runnable2* tasks2[] = { &radio2, &signalIn3, &iax2Channel1, &bridge10, &webUi, 
        &cfgPoller };
    EventLoop::run(log, clock, 0, 0, tasks2, std::size(tasks2), nullptr, false);

    return 0;
}

/** 
 * A crash signal handler that displays stack information on stdout
 */
static void sigHandler(int sig) {
    void *array[64];
    size_t size = backtrace(array, 64);
    fprintf(stderr, "=========================================================\n");
    fprintf(stderr, "IMPORTANT: Save this stack trace for analysis!\n\n");
    fprintf(stderr, "Error signal %d:\n", sig);
    fprintf(stderr, "Version %s Git Hash %s\n\n", VERSION, GIT_HASH);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    fprintf(stderr, "\naddr2line -r ./amp-server -fC <addr>\n\n");
    fprintf(stderr, "=========================================================\n");
    // Now do the regular thing
    signal(sig, SIG_DFL); 
    raise(sig);
}
