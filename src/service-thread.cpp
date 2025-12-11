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

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/linux/StdClock.h"

#include "RegisterTask.h"
#include "StatsTask.h"
#include "EventLoop.h"

using namespace std;
using namespace kc1fsz;

void* service_thread(void* l) {

    pthread_setname_np(pthread_self(), "SVC");

    Log& log = *((Log*)l);
    log.info("Start serice_thread");
    StdClock clock;

    RegisterTask registerTask(log, clock);
    registerTask.configure(getenv("AMP_ASL_REG_URL"), getenv("AMP_NODE0_NUMBER"), 
        getenv("AMP_NODE0_PASSWORD"), atoi(getenv("AMP_IAX_PORT")));

    // #### TODO: CLEAN UP
    // Sleep waiting to change real-time status
    sleep(10);

    const pthread_t self_thread = pthread_self();
    int policy;
    struct sched_param param;

    // Get current scheduling parameters
    if (pthread_getschedparam(self_thread, &policy, &param) != 0) {
        perror("pthread_getschedparam failed");
        return 0;
    }
    if (policy != SCHED_OTHER) {
        // Set the new policy to SCHED_OTHER and priority to 0 (required for SCHED_OTHER)
        param.sched_priority = 0;
        if (pthread_setschedparam(self_thread, SCHED_OTHER, &param) != 0) {
            perror("pthread_setschedparam to SCHED_OTHER failed");
            // Check for EPERM if it was previously a real-time policy and no capabilities are set
            if (errno == EPERM) {
                printf("Permission denied. Ensure the process has CAP_SYS_NICE if changing from a real-time policy.\n");
            }
            return 0;
        }
        // Get current scheduling parameters
        if (pthread_getschedparam(self_thread, &policy, &param) != 0) {
            perror("pthread_getschedparam failed");
            return 0;
        }
        log.info("Thread policy: %d, priority: %d", policy, param.sched_priority);
    }

    // Main loop        
    const unsigned task2Count = 1;
    Runnable2* tasks2[task2Count] = { &registerTask };
    EventLoop::run(log, clock, 0, 0, tasks2, task2Count);

    return 0;
}
