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
#ifndef _WIN32
// This stuff is needed for the thread priority manipulation
#include <sched.h>
#include <linux/sched.h>
#include <linux/sched/types.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h> 
#include <alsa/asoundlib.h>
#include <execinfo.h>
#include <signal.h>
#endif

// Needed for DNS lookup
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <chrono>
#include <thread>

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/linux/StdClock.h"
#include "kc1fsz-tools/MicroDNS.h"
#include "kc1fsz-tools/NetUtils.h"

#include "RegisterTask.h"
#include "StatsTask.h"
#include "EventLoop.h"
#include "ConfigPoller.h"
#include "ThreadUtil.h"
#include "TimerTask.h"

#include "service-thread.h"

using namespace std;
using namespace kc1fsz;

static const char* POKE_HOST_NAME = "61057.nodes.allstarlink.org";
static int POKE_PORT = 4570;

namespace kc1fsz {
    namespace amp {

/**
 * @param reqQueue A queue of inbound requests from other threads.
 */
void serviceThread(const std::string* cfgFileName, kc1fsz::Log* loga,
    const char* version, copyableatomic<std::string>* pokeAddr,
    threadsafequeue2<MessageCarrier>* reqQueue) {

    Log& log = *loga;

    amp::setThreadName("amp-svc");

    log.info("service_thread start");

    // Sleep waiting to change real-time status
    std::this_thread::sleep_for(std::chrono::seconds(10));
    amp::lowerThreadPriority();

    StdClock clock;

    // This task is responsible for periodically re-asserting the node's registration
    RegisterTask registerTask(log, clock);

    // This task is responsible for periodically re-posting the node's statistics
    StatsTask statsTask(log, clock, version);

    // This timer task checks for messages on the request queue 
    // and distributes them appropriately.
    TimerTask timer2(log, clock, 5, 
        [&log, reqQueue, &statsTask]() {
            MessageCarrier msg;
            if (reqQueue->try_pop(msg, 10)) {
                // The node list for stats reporting
                if (msg.isSignal(Message::SignalType::LINK_REPORT))
                    statsTask.setNodeList((const char*)msg.body());
            }
        }
    );

    // This task looks at the configuration document and re-processes it when it 
    // changes.
    amp::ConfigPoller cfgPoller(log, cfgFileName->c_str(), 
        // This function will be called on any update to the configuration document.
        [&log, &registerTask, &statsTask](const json& cfg) {
            try {
                if (!cfg["aslRegUrl"].is_string())
                    throw invalid_argument("aslRegUrl is missing/invalid");
                if (!cfg["aslStatUrl"].is_string())
                    throw invalid_argument("aslStatUrl is missing/invalid");
                if (!cfg["iaxPort"].is_string())
                    throw invalid_argument("iaxPort is missing/invalid");
                if (!cfg["node"].is_string())
                    throw invalid_argument("node is missing/invalid");
                if (!cfg["password"].is_string())
                    throw invalid_argument("password is missing/invalid");

                registerTask.configure(
                    cfg["aslRegUrl"].get<std::string>().c_str(), 
                    cfg["node"].get<std::string>().c_str(), 
                    cfg["password"].get<std::string>().c_str(), 
                    std::stoi(cfg["iaxPort"].get<std::string>()));

                statsTask.configure(
                    cfg["aslStatUrl"].get<std::string>().c_str(), 
                    cfg["node"].get<std::string>().c_str());
            }
            catch (exception& ex) {
                log.error("Failed to load configuration: %s", ex.what());
            }
        }
    );

    // This is a timer that does a DNS resolution on the "poke node"
    // periodically and puts the result into the pokeAddr that is 
    // shared with the main thread.
    TimerTask timer1(log, clock, 120, 
        [&log, pokeAddr]() {
            // DNS lookup
            unsigned char answer[128];
            int answerLen;
            answerLen = res_query(POKE_HOST_NAME, 1, 1, answer, sizeof(answer));
            if (answerLen < 0) {
                log.error("Unable to resolve address for %s", POKE_HOST_NAME);
                return;
            }
            uint32_t addr;
            int rc2 = microdns::parseDNSAnswer_A(answer, answerLen, &addr);
            if (rc2 < 0) {
                log.error("Unable to resolve address for %s (2)", POKE_HOST_NAME);
                return;
            }
            char dottedAddr[32];
            formatIP4Address(addr, dottedAddr, sizeof(dottedAddr));
            char addrAndPort[64];
            snprintf(addrAndPort, 64, "%s:%d", dottedAddr, POKE_PORT);
            pokeAddr->set(string(addrAndPort));
        }
    );

    // Main loop        
    Runnable2* tasks2[] = { &registerTask, &statsTask, &cfgPoller, &timer1, &timer2 };
    EventLoop::run(log, clock, 0, 0, tasks2, std::size(tasks2));

    // #### TODO: NEED A CLEAN WAY TO EXIT THIS THREAD
}

    }
}
