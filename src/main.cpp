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

#include <iostream>
#include <cmath> 
#include <queue>

#include <curl/curl.h>

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/linux/StdClock.h"
#include "kc1fsz-tools/linux/MTLog.h"
#include "kc1fsz-tools/fixedqueue.h"
#include "kc1fsz-tools/NetUtils.h"

#include "sound-map.h"

#include "LineIAX2.h"
#include "LineUsb.h"
#include "MessageBus.h"
#include "ManagerTask.h"
#include "EventLoop.h"
#include "AdaptorOut.h"
#include "AdaptorIn.h"
#include "service-thread.h"

using namespace std;
using namespace kc1fsz;

/*
Development:

export AMP_NODE0_NUMBER=672730
export AMP_NODE0_PASSWORD=xxxx
export AMP_NODE0_MGR_PORT=5038
export AMP_IAX_PORT=4568
export AMP_IAX_PROTO=IPV4
export AMP_ASL_REG_URL=https://register.allstarlink.org
export AMP_NODE0_USBSOUND="vendorname:\"C-Media Electronics, Inc.\""
*/
extern uint32_t cmsisdsp_overflow;

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

class MessageBus2 : public MessageConsumer {
public:

    MessageBus2(Log& log) : _log(log) { }

    virtual void consume(const Message& msg) {
        if (msg.getType() == Message::Type::SIGNAL && 
            msg.getFormat() == Message::SignalType::CALL_START) {
            PayloadCallStart payload;
            assert(msg.size() == sizeof(payload));
            memcpy(&payload, msg.raw(), sizeof(payload));
            _log.info("Call started %d codec %X", msg.getSourceCallId(), payload.codec);
            adIn->setCodec(payload.codec);
            adOut->setCodec(payload.codec);
        }
        adIn->consume(msg);
    }

    Log& _log;
    AdaptorIn* adIn = 0;
    AdaptorOut* adOut = 0;
};

// A crash signal handler that displays stack information
static void sigHandler(int sig) {
    void *array[32];
    // get void*'s for all entries on the stack
    size_t size = backtrace(array, 32);
    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    // Now do the regular thing
    signal(sig, SIG_DFL); 
    raise(sig);
}

/*
Topology:

iax0->mb0->adaptor0->radio0
radio0->adaptor1->iax0
*/
int main(int argc, const char** argv) {

    // Name the thread
    pthread_setname_np(pthread_self(), "M  ");
    signal(SIGSEGV, sigHandler);

    MTLog log;
    log.info("Start main");
    StdClock clock;

    // Get libcurl going
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res) {
        log.error("Libcurl failed");
        return 0;
    }

    // Get the service thread running
    pthread_t new_thread_id;
    if (pthread_create(&new_thread_id, NULL, service_thread, (Log*)(&log)) != 0) {
        perror("Error creating thread");
        return -1;
    }
    
    // This goes from IAX2->USB
    MessageBus2 mb0(log);
    AdaptorIn adaptor0;
    // This goes from USB->IAX2
    AdaptorOut adaptor1;

    // Resolve the sound card/HID name
    char alsaCardNumber[16];
    char hidDeviceName[32];
    int rc2 = querySoundMap(getenv("AMP_NODE0_USBSOUND"), 
        hidDeviceName, 32, alsaCardNumber, 16, 0, 0);
    if (rc2 < 0) {
        log.error("Unable to resolve USB device %d", rc2);
        return -1;
    }
    char alsaDeviceName[32];
    snprintf(alsaDeviceName, 32, "plughw:%s", alsaCardNumber);

    log.info("USB %s mapped to %s, %s", getenv("AMP_NODE0_USBSOUND"),
        hidDeviceName, alsaDeviceName);

    LineUsb radio0(log, clock, adaptor1, 2, 1, 3, Message::BROADCAST);
    int rc = radio0.open(alsaDeviceName, hidDeviceName);
    if (rc < 0) {
        log.error("%d", rc);
        return -1;
    }

    CallValidatorStd val;
    LocalRegistryStd locReg;
    LineIAX2 iax2Channel0(log, clock, 1, mb0, &val, &locReg);
    //iax2Channel0.setTrace(true);

    // Routes
    mb0.adIn = &adaptor0;
    mb0.adOut = &adaptor1;
    adaptor0.setSink([&radio0](const Message& msg) { radio0.consume(msg); });
    adaptor1.setSink([&iax2Channel0](const Message& msg) { iax2Channel0.consume(msg); });

    // The listening node
    iax2Channel0.open(AF_INET, atoi(getenv("AMP_IAX_PORT")), "radio");
    
    ManagerSink mgrSink(iax2Channel0);
    ManagerTask mgrTask(log, clock, atoi(getenv("AMP_NODE0_MGR_PORT")));
    mgrTask.setCommandSink(&mgrSink);

    // Main loop        
    const unsigned task2Count = 3;
    Runnable2* tasks2[task2Count] = { &radio0, &iax2Channel0, &mgrTask };

    EventLoop::run(log, clock, 0, 0, tasks2, task2Count, nullptr, true);

    iax2Channel0.close();
    radio0.close();

    log.info("Done");

    return 0;
}
