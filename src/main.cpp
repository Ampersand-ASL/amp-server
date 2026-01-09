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
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <execinfo.h>
#include <signal.h>

#include <iostream>
#include <queue>
#include <stdexcept>

#include <curl/curl.h>
#include <argparse/argparse.hpp>

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/linux/StdClock.h"
#include "kc1fsz-tools/linux/MTLog.h"
#include "kc1fsz-tools/fixedqueue.h"
#include "kc1fsz-tools/NetUtils.h"

#include "sound-map.h"

#include "LineIAX2.h"
#include "LineUsb.h"
#include "ManagerTask.h"
#include "EventLoop.h"
#include "Bridge.h"
#include "BridgeCall.h"
#include "MultiRouter.h"
#include "WebUi.h"
#include "ConfigPoller.h"
#include "SignalIn.h"
#include "ThreadUtil.h"
#include "service-thread.h"
#include "TraceLog.h"

using namespace std;
using namespace kc1fsz;

static const char* VERSION = "20260109.0";
// ### TODO: FIGURE THIS OUT
const char* const GIT_HASH = "?";

// Connects the manager to the IAX channel (TEMPORARY)
class ManagerSink : public ManagerTask::CommandSink {
public:

    ManagerSink(LineIAX2& ch) 
    :   _ch(ch) { }

    void execute(const char* cmd) { 
        _ch.processManagementCommand(cmd);
    }

private:

    LineIAX2& _ch;
};

class CallValidatorStd : public CallValidator {
public:
    virtual bool isNumberAllowed(const char* targetNumber) const {
        return true;
    }
};

class LocalRegistryStd : public LocalRegistry {
public:
    virtual bool lookup(const char* destNumber, sockaddr_storage& addr) {
        /*
        //addr.ss_family = AF_INET6;
        addr.ss_family = AF_INET;
        //setIPAddr(addr, "::1");
        setIPAddr(addr, "127.0.0.1");
        setIPPort(addr, 4569);
        char temp[64];
        formatIPAddrAndPort((const sockaddr&)addr, temp, 64);
        return true;
        */
       return false;
    }
};

// A crash signal handler that displays stack information
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
        else 
            log.error("Unable to create default configuration");
    }

    // Get the service thread running (handles registration, stats, etc.)
    service_thread_args args1;
    args1.log = &log;
    args1.cfgFileName = cfgFileName;
    std::thread serviceThread(service_thread, &args1);

    // This is the "bus" that passes messages between components
    MultiRouter router;

    // The bridge is what provides the conference
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
    LineIAX2 iax2Channel1(log, clock, 1, router, &val, &locReg, 10);
    router.addRoute(&iax2Channel1, 1);
    if (program["--trace"] == true)
        iax2Channel1.setTrace(true);

    // Instantiate the server for the web-based UI
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

            // Transfer the new configuration into the various places it is needed
            webUi.setConfig(cfg);

            try {
                //iax2Channel1.setPrivateKey(getenv("AMP_PRIVATE_KEY"));
                //iax2Channel1.setDNSRoot(getenv("AMP_ASL_DNS_ROOT"));
                
                if (!cfg["iaxPort"].is_string())
                    throw invalid_argument("iaxPort is missing/invalid");

                int rc;
                rc = iax2Channel1.open(AF_INET, std::stoi(cfg["iaxPort"].get<std::string>()), "radio");
                if (rc < 0) {
                    log.error("Failed to open IAX2 connection %d", rc);
                }

                string setupMode = cfg["setupMode"].get<std::string>();

                // ----- ASL Compatibility Mode -----------------------------------

                if (setupMode.empty() || setupMode == "0") {

                    // Resolve the audio device
                    string aslAudioDevice = cfg["aslAudioDevice"].get<std::string>();
                    if (aslAudioDevice.starts_with("usb ")) {
                        int alsaCard;
                        string ossDevice;
                        int rc2 = querySoundMap(aslAudioDevice.substr(4).c_str(), alsaCard, ossDevice);
                        if (rc2 < 0) {
                            log.error("Unable to resolve sound device %d", rc2);
                        } 
                        else {
                            log.info("Audio %s mapped to ALSA card %d", 
                                aslAudioDevice.c_str(), alsaCard);                         

                            // NOTE: ASL uses 0-1000 scale
                            if (!cfg["aslTxMixASet"].is_string())
                                throw invalid_argument("aslTxMixASet is missing/invalid");
                            int txMixASet = std::stoi(cfg["aslTxMixASet"].get<std::string>());

                            if (!cfg["aslTxMixBSet"].is_string())
                                throw invalid_argument("aslTxMixBSet is missing/invalid");
                            int txMixBSet = std::stoi(cfg["aslTxMixBSet"].get<std::string>());
                            
                            if (!cfg["aslRxMixerSet"].is_string())
                                throw invalid_argument("aslRxMixerSet is missing/invalid");
                            int rxMixerSet = std::stoi(cfg["aslRxMixerSet"].get<std::string>());

                            rc = radio2.open(alsaCard, txMixASet, txMixBSet, rxMixerSet);
                            if (rc < 0) {
                                if (rc == -12)
                                    log.error("Unable to open sound device, busy");
                                else 
                                    log.error("Unable to open sound device");
                                return;
                            }
                        }
                    }

                    // Resolve the COS signal
                    string aslCosFrom = cfg["aslCosFrom"].get<std::string>();
                    if (aslAudioDevice.starts_with("usb ") && aslCosFrom.starts_with("usb")) {

                        string cosSignalDevice;
                        int rc3 = queryHidMap(aslAudioDevice.substr(4).c_str(), cosSignalDevice);
                        if (rc3 < 0) {
                            log.error("Unable to resolve HID device %d", rc3);
                        } 
                        else {
                            log.info("HID %s mapped to %s", aslAudioDevice.c_str(),
                                cosSignalDevice.c_str());
                            rc = signalIn3.openHid(cosSignalDevice.c_str());
                            if (rc < 0) {
                                log.error("Failed to open HID signal connection %d", rc);
                                return;
                            }
                        }

                        // ##### TODO: DEAL WITH INVERT
                    }
                }
            }
            // ### TODO MORE SPECIFIC
            catch (json::exception& ex) {
                log.error("Failed to process configuration change %s", ex.what());
            }
        }
    );

    //ManagerSink mgrSink(iax2Channel1);
    //ManagerTask mgrTask(log, clock, atoi(getenv("AMP_NODE0_MGR_PORT")));
    //mgrTask.setCommandSink(&mgrSink);

    // Main loop        
    Runnable2* tasks2[] = { &radio2, &signalIn3, &iax2Channel1, &bridge10, &webUi, &cfgPoller };
    EventLoop::run(log, clock, 0, 0, tasks2, std::size(tasks2), nullptr, false);

    return 0;
}

