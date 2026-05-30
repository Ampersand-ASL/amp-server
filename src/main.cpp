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
#include <syslog.h>
#include <iostream>

// 3rd party HTTP/HTTPS client
#include <curl/curl.h>
// 3rd party command-line parser
#include <argparse/argparse.hpp>

// Non-AMP stuff from my C++ tools library
#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/linux/StdClock.h"
#include "kc1fsz-tools/MTLog2.h"
#include "kc1fsz-tools/threadsafequeue2.h"

// All of this comes from AMP Core
#include "TraceLog.h"
#include "EventLoop.h"
#include "ThreadUtil.h"
#include "service-thread.h"
#include "MultiRouter.h"
#include "LineIAX2.h"
#include "LineUsb.h"
#include "LineSDRC.h"
#include "Bridge.h"
#include "BridgeCall.h"
#include "WebUi.h"
#include "ConfigPoller.h"
#include "SignalIn.h"
#include "SignalOut.h"
#include "TimerTask.h"
#include "QueueConsumer.h"
#include "TTSServiceSimple.h"

// And a few things from AMP Server
#include "LocalRegistryStd.h"
#include "LocalAuthenticatorStd.h"
#include "config-handler.h"

#define MAX_CALLS (8)

using namespace std;
using namespace kc1fsz;

// ### TODO: FIGURE OUT HOW TO MAKE THIS AUTOMATIC
static const char* VERSION = "20260530.0";
static const char* const GIT_HASH = "?";
static const char* PUBLIC_USER = "radio";

// Line IDs
#define LINE_ID_IAX (1)
#define LINE_ID_STATS (12)
#define LINE_ID_SIGNAL_OUT (31)
#define LINE_ID_BRIDGE (10)
#define LINE_ID_TTS_SIMPLE (19)

static void sigHandler(int sig);

// These are potentially large structure, so keeping it off the stack
static amp::BridgeCall callBank[MAX_CALLS];
static LineIAX2::Call iaxCallBank[MAX_CALLS];

static void logCb(const char* sev, const char* dt, const char* msg) {
    if (sev[0] == 'E') {
        syslog(LOG_ERR, msg);
    }
    std::cout << sev << " " << dt << " " << msg << std::endl;
}

int main(int argc, const char** argv) {

    // Name the thread
    amp::setThreadName("amp-server");
    // Install the crash stack handler
    signal(SIGSEGV, sigHandler);

    // Open a connection to the system logger
    // LOG_PID includes the process ID in each entry
    // LOG_CONS It instructs the program to write log messages directly to the 
    //   system console (/dev/console) as a fallback if it fails to send them to 
    //   the main syslog daemon.
    // LOG_USER identifies the facility (type of program)
    openlog("amp-server", LOG_PID | LOG_CONS, LOG_USER);

    // Create a logger that holds onto some history for display purposes.
    // #### TODO: Think about the performance implications of the lock that 
    // #### is acquired when the UI thread reads the log.
    MTLog2 log(logCb);

    log.info("AMP Server");
    log.info("Powered by the Ampersand ASL Project https://github.com/Ampersand-ASL");
    log.info("Copyright (C) 2026, Bruce MacKinnon KC1FSZ");
    log.info("Version %s Git Hash %s", VERSION, GIT_HASH);
    log.info("----------------------------------------------------------------------");

    // syslog startup stuff
    syslog(LOG_INFO,"AMP Server startup %s", VERSION);

    StdClock clock;

    // A special log used for tracing/performance analysis
    const unsigned traceLogDataLen = 1024;
    std::string traceLogData[traceLogDataLen];
    TraceLog traceLog(clock, traceLogData, traceLogDataLen);

    // Get libcurl going
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res) {
        log.error("Libcurl failed to initialize %d", res);
        std::exit(-1);
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

    string uiPwd;
    program.add_argument("--httppwd")
        .help("Password for HTTP UI/API authentication")
        .store_into(uiPwd);

    program.add_argument("--trace")
        .help("Turn on network tracing")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--capture")
        .help("Turn on network capture")
        .default_value(false)
        .implicit_value(true);

    int iaxPort = 0;
    program.add_argument("--iaxport")
        .store_into(iaxPort)
        .default_value(0)
        .help("IAX port, overrides system configuration");

    string callNode;
    program.add_argument("--callnode")
        .help("Node to call immediately")
        .store_into(callNode);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        log.error("Argument error: %s", err.what());
        std::exit(-2);
    }

    log.info("Using configuration file %s", cfgFileName.c_str());

    // Create a default/starting config file if this is the first time.
    if (!filesystem::exists(cfgFileName)) {
        log.info("Creating default configuration");
        ofstream cfg(cfgFileName);
        if (cfg.is_open()) 
            cfg << "{}" << endl;
        else {
            log.error("Unable to create default configuration");
            std::exit(-3);
        }
    }

    // A queue used by other threads to pass messages into the main thread's
    // router.
    threadsafequeue2<MessageCarrier> respQueue;
    // A wrapper that makes the response queue look like a MessageConsumer.
    // We would use this **outside of the main thread** to put things onto the 
    // respQueue declared above.
    QueueConsumer respQueueConsumer(respQueue);

    // This is the router (aka "bus") that passes Message objects between the rest 
    // of the components in the system. You'll see that everything else below is
    // wired to the router one way or the other.
    MultiRouter router(respQueue);

    copyableatomic<std::string> pokeAddr;

    // Setup a way to pass messages over to the service thread
    threadsafequeue2<MessageCarrier> serviceThreadReqQueue;
    QueueConsumer serviceThreadReqQueueConsumer(serviceThreadReqQueue);
    // Pass message from local router up to the service thread
    router.addRoute(&serviceThreadReqQueueConsumer, LINE_ID_STATS);

    // Get the service thread running. This handles non-time-sensitive
    // stuff like registration, stats, etc.
    std::thread serviceThread(amp::serviceThread, &cfgFileName, &log, VERSION, 
        &pokeAddr, &serviceThreadReqQueue);

    // The Bridge is what provides the audio conference capability. The various 
    // Lines connect to the Bridge.
    amp::Bridge bridge10(log, traceLog, clock, router, amp::BridgeCall::Mode::NORMAL, 10, 
        0, 0, 0, 1, LINE_ID_STATS, LINE_ID_TTS_SIMPLE, callBank, MAX_CALLS);
    router.addRoute(&bridge10, 10);

    // This is the Line that connects to the USB sound interface
    LineUsb radio2(log, clock, router, 2, 1, LINE_ID_BRIDGE, Message::UNKNOWN_CALL_ID, 
        LINE_ID_SIGNAL_OUT, LINE_ID_IAX);
    router.addRoute(&radio2, 2);

    // This manages the COS signal detect
    amp::SignalIn signalIn3(log, clock, router, 2, 
        Message::SignalType::COS_ON, Message::SignalType::COS_OFF);

    // This manages the PTT signal generation
    amp::SignalOut signalOut31(log, clock, router, 
        Message::SignalType::PTT_ON, Message::SignalType::PTT_OFF);
    router.addRoute(&signalOut31, LINE_ID_SIGNAL_OUT);

    // This manages the interface to the SDRC (if any)
    LineSDRC sdrcLine5(log, traceLog, clock, 5, 1, router, 10);
    router.addRoute(&sdrcLine5, 5);

    // This is the Line that makes the IAX2 network connection
    LocalRegistryStd locReg;
    LocalAuthenticatorStd locAuth(log);
    LineIAX2 iax2Channel1(log, traceLog, clock, 1, router, 0, 0, &locReg, 
        &locAuth,
        10, PUBLIC_USER, iaxCallBank, MAX_CALLS);
    router.addRoute(&iax2Channel1, 1);
    if (program["--trace"] == true)
        iax2Channel1.setTrace(true);
    if (program["--capture"] == true)
        iax2Channel1.setCapture(true);
    iax2Channel1.setPokeEnabled(true);
    iax2Channel1.setPokeAddr("52.8.197.124:4570");
    iax2Channel1.setDirectedPokeEnabled(true);

    // This is the HTTP server that provides the UI
    amp::WebUi webUi(log, clock, uiPort, 1, 2, 
        cfgFileName.c_str(), VERSION, traceLog);
    // This allow the WebUi to watch all traffic and pull out the things 
    // that are relevant for status display.
    router.addRoute(&webUi, MultiRouter::BROADCAST);   
    webUi.setUiPWd(uiPwd);

    // Get the UI thread going. 
    std::thread webUiThread(amp::WebUi::uiThread, &webUi, &respQueueConsumer);

    // This is a poller that watches for changes to the configuration file
    // and applies those changes to everything on the main thread.
    amp::ConfigPoller cfgPoller(log, cfgFileName.c_str(), 
        // This function will be called on any update to the configuration document.
        [&log, &webUi, &iax2Channel1, &locReg, &locAuth, &radio2, &signalIn3, &signalOut31, 
        &bridge10, &sdrcLine5,
         iaxPort]
        (const json& cfg) {

            log.info("Configuration change detected");
            cout << cfg.dump() << endl;

            try {
                amp::configHandler(log, cfg, webUi, iax2Channel1, locReg, locAuth, radio2, signalIn3, 
                    signalOut31, bridge10, sdrcLine5, iaxPort);
            }
            // ### TODO MORE SPECIFIC
            catch (json::exception& ex) {
                log.error("Failed to process configuration change %s", ex.what());
                return;
            }
        },
        // This function will be called once on startup
        [&log, &iax2Channel1, &callNode]
        (const json& cfgDoc) {
            string localNode;
            if (cfgDoc.contains("node"))
                localNode = cfgDoc["node"];
            // If a node was specified on the command line then connect immediately
            if (!localNode.empty() && !callNode.empty()) {
                iax2Channel1.call(localNode.c_str(), callNode.c_str());
            }
        }
    );

    // Setup a poller that looks at the bridge status and passes any updates
    // over to the web UI. We will get an event *AT LEAST* every 10 seconds.
    amp::BridgeStatusDocPoller statusPoller(log, clock, bridge10, 10 * 1000,
        [&webUi](const json& statusDoc) {
            webUi.setBridgeStatus(statusDoc);
        }
    );

    // Setup a timer that takes the poke address generated from the service
    // thread and puts it into the IAX line.
    TimerTask timer1(log, clock, 10, 
        [&log, &pokeAddr, &iax2Channel1]() {
            std::string addr = pokeAddr.getCopy();
            if (!addr.empty())
                iax2Channel1.setPokeAddr(addr.c_str());
        }
    );

    TTSServiceSimple ttsSimple19(log, clock, router, LINE_ID_TTS_SIMPLE,
        LINE_ID_BRIDGE);
    router.addRoute(&ttsSimple19, LINE_ID_TTS_SIMPLE);

    // Setup the EventLoop with all of the tasks that need to be run on this thread
    Runnable2* tasks[] = { &radio2, &signalIn3, &signalOut31, &iax2Channel1, &bridge10, &webUi, 
        &cfgPoller, &sdrcLine5, &timer1, &statusPoller, &ttsSimple19, &router };
    EventLoop::run(log, clock, 0, 0, tasks, std::size(tasks), nullptr, true);

    // #### TODO: At the moment there is no clean way to get out of the loop

    std::exit(0);
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
