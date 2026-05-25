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

// All of this comes from AMP Core
#include "EventLoop.h"
#include "TimerTask.h"
#include "RegisterTaskIAX2.h"

using namespace std;
using namespace kc1fsz;

int main(int argc, const char** argv) {

    Log log;
    StdClock clock;

    log.info("Ampersand Registration Test");
    log.info("Bruce MacKinnon KC1FSZ");

    RegisterTaskIAX2 regTask(log, clock);
    regTask.configure("register.allstarlink.org:4568", "672732", "microlink2", 4568);

    // Setup the EventLoop with all of the tasks that need to be run on this thread
    Runnable2* tasks[] = { &regTask };
    EventLoop::run(log, clock, 0, 0, tasks, std::size(tasks), nullptr, true);
}
