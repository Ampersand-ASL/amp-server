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

// The goal of this program is provide a MINIMALISTIC but REALISTIC demonstration
// of the USB audio path used in Ampersand. The only "fake" part of the system is
// the test audio generator.
//
// Please see amp-core/src/test/TestAudioGenerator to adjust the actual audio 
// streams being used.

#include <iostream>

// Non-AMP stuff from my C++ tools library
#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/linux/StdClock.h"

// A library with USB mapping stuff
#include "sound-map.h"

// All of this comes from AMP Core
#include "EventLoop.h"
#include "LineUsb.h"
#include "TimerTask.h"
#include "MultiRouter.h"
#include "tests/TestAudioGenerator.h"

// Line IDs of the various major components in the system. Used to 
// route messages through the bus.
#define LINE_ID_IAX (1)
#define LINE_ID_RADIO (2)
#define LINE_ID_SIGNAL_OUT (31)
#define LINE_ID_BRIDGE (10)

#define CMEDIA_VENDOR_ID ("0d8c")

using namespace std;
using namespace kc1fsz;

// Mixer levels
static int txMixASet = 0;
static int txMixBSet = 0;
static int rxMixerSet = 0;

static const char* defaultPort = "3-2.2";

int main(int argc, const char** argv) {

    Log log;
    StdClock clock;

    log.info("Ampersand Audio Test 2");
    log.info("Bruce MacKinnon KC1FSZ");

    // Try to find a CM108 to use for audio output

    char port[32];
    strcpy(port, defaultPort);
    visitUSBDevices2([&port, &log](
        const char* vendorName, const char* productName, 
        const char* vendorId, const char* productId,                 
        const char* portPath, int, int) {
            if (strcasecmp(vendorId, CMEDIA_VENDOR_ID) == 0) {
                strcpy(port, portPath);
                log.info("Audio device found at port %s", port);
            }
        }
    );
    // Check for user-supplied override
    if (argc > 1) {
        strcpy(port, argv[1]);
    }

    // Resolve the USB port path into an actual ALSA card
    int alsaCard;
    string dummyOssDevice;
    int rc2 = resolveUSBSoundDevice(port, alsaCard, dummyOssDevice);
    if (rc2 < 0) {
        log.error("Unable to resolve audio device %d", rc2);
        return -1;
    } 

    log.info("Audio device %s mapped to ALSA card %d", port, alsaCard);                         

    // A queue used by other threads to pass messages into the main thread's
    // router.
    threadsafequeue2<MessageCarrier> respQueue;

    // This is the router (aka "bus") that passes Message objects between the rest 
    // of the components in the system. You'll see that everything else below is
    // wired to the router one way or the other.
    MultiRouter router(respQueue);

    // This is the Line that connects to the USB sound interface
    LineUsb radio2(log, clock, router, LINE_ID_RADIO, 1, LINE_ID_BRIDGE, Message::UNKNOWN_CALL_ID, 
        LINE_ID_SIGNAL_OUT, LINE_ID_IAX);
    router.addRoute(&radio2, 2);

    // Open the audio line
    int rc = radio2.open(alsaCard, txMixASet, txMixBSet, rxMixerSet, false, 0);
    if (rc < 0) {
        if (rc == -12)
            log.error("Unable to open sound device, busy");
        else 
            log.error("Unable to open sound device");
        return -1;
    }

    // The only "fake" part of the system is here. This is responsible for generating
    // test audio. Audio is a stream of Message objects injected onto the bus.
    TestAudioGenerator audioGen(log, clock, router, LINE_ID_RADIO);
    
    // Setup the EventLoop with all of the tasks that need to be run on this thread
    Runnable2* tasks[] = { &radio2, &router, &audioGen };
    EventLoop::run(log, clock, 0, 0, tasks, std::size(tasks), nullptr, true);
}
