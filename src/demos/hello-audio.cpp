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

// A test program to validate correct timing of USB/ALSA API.
// Will operate using the first 


#include <iostream>
#include <cmath> 
#include <chrono>
#include <thread>

#include <alsa/asoundlib.h>

#include "kc1fsz-tools/Log.h"
#include "kc1fsz-tools/StdPollTimer.h"
#include "kc1fsz-tools/linux/StdClock.h"

#include "sound-map.h"

#define CMEDIA_VENDOR_ID ("0d8c")

static const char* defaultPort = "3-2.2";
static const unsigned bufferMs = 5;

using namespace std;
using namespace kc1fsz;

int main(int argc, const char** argv) {

    Log log;
    StdClock clock;
    StdPollTimer timer20ms(clock, 20000);
    int rc;

    log.info("Ampersand Audio Test");
    log.info("Bruce MacKinnon KC1FSZ");

    char port[32];
    strcpy(port, defaultPort);

    // Try to find a CM108
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

    int alsaCard;
    string ossDevice;
    int rc2 = resolveUSBSoundDevice(port, alsaCard, ossDevice);
    if (rc2 < 0) {
        log.error("Unable to resolve audio device %d", rc2);
        return -1;
    } 

    log.info("Audio device %s mapped to ALSA card %d", port, alsaCard);                         

    char alsaDeviceName[16];
    snprintf(alsaDeviceName, 16, "plughw:%d,0", alsaCard);
    snd_pcm_t* playH = 0;

    if ((rc = snd_pcm_open(&playH, alsaDeviceName, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK)) < 0) {
        if (rc == -16) {
            log.error("Can't open sound device %s, busy", alsaDeviceName);
            return -12;
        } else {
            log.error("Cannot open playback device %s %d", alsaDeviceName, rc);
            return -10;
        }
    }

    const unsigned int audioRate = 48000;
    unsigned int channels = 2;

    // No free needed, alloca() frees memory one function exit
    snd_pcm_hw_params_t* play_hw_params;
    snd_pcm_hw_params_alloca(&play_hw_params);
    snd_pcm_hw_params_any(playH, play_hw_params);
    snd_pcm_hw_params_set_access(playH, play_hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(playH, play_hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate(playH, play_hw_params, audioRate, 0);
    snd_pcm_hw_params_set_channels_near(playH, play_hw_params, &channels);
    unsigned int periodTimeUs = 20000;
    snd_pcm_hw_params_set_period_time_near(playH, play_hw_params, &periodTimeUs, 0);
   
    // Let the buffer store up to 4x 20ms frames of sound. 
    // At 48K, there are 960 samples in a 20ms frame.
    // NOTE: This has been checked and it is working. Nothing here adds delay to the 
    // playout, it just determines the maximum amount of delay that can be supported.
    unsigned int bufferTimeUs = 20000 * 4;
    snd_pcm_hw_params_set_buffer_time_near(playH, play_hw_params, &bufferTimeUs, 0);

    if ((rc = snd_pcm_hw_params(playH, play_hw_params)) < 0) {
        log.error("Unable to configure play HW parameters %d", rc);
        return -1;
    }
    
    log.info("USB buffer size %u (us)", bufferTimeUs);

    snd_pcm_sw_params_t* play_sw_params;
    snd_pcm_sw_params_alloca(&play_sw_params);
    snd_pcm_sw_params_current(playH, play_sw_params);
    // Set the start threshold at half of the buffer
    unsigned int startThreshold = 960 + ((960 * bufferMs) / 20);
    snd_pcm_sw_params_set_start_threshold(playH, play_sw_params, startThreshold);

    log.info("Start threshold %u (frames)", startThreshold);

    if ((rc = snd_pcm_sw_params(playH, play_sw_params)) < 0) {
        log.error("Unable to configure play SW parameters %d", rc);
        return -1;
    }

    // Get the FD's 
    unsigned fdsCapacity = 2;
    pollfd fds[2];
    //unsigned pollCount = 0;
    rc = snd_pcm_poll_descriptors(playH, fds, fdsCapacity);
    if (rc < 0) {
        log.error("FD problem");
        return -1;
    } 
    //pollCount = rc;

    // Get the initial state of the driver
    snd_pcm_state_t lastState;
    {
        snd_pcm_status_t *status = 0;
        snd_pcm_status_alloca(&status);
        snd_pcm_status(playH, status);
        lastState = snd_pcm_status_get_state(status);
    }

    // Make a test tone that we can transmit
    const float testToneHz = 1000;
    const float omega = 2.0f * 3.1415926f * testToneHz / (float)audioRate;
    const float amp = 0.25;
    const unsigned testToneSize = 48000 * 2;
    float phi = 0;
    int16_t testTone[testToneSize];

    for (unsigned i = 0; i < testToneSize; i++) {
        float sample = amp * cos(phi);
        phi += omega;
        testTone[i] = 32767.0f * sample;
    }

    // Used to track phase through the test tone
    unsigned testTonePtr = 0;
    unsigned lastWriteSize = 0;
    unsigned loop = 0;
    bool toneActive = false;
    unsigned lastDelayFrames = 0;

    // Main event loop
    while (true) {

        if (timer20ms.poll()) {

            if (loop % 150 == 0) {
                log.info("Turning on tone");
                toneActive = true;
            }
            else if (loop % 150 == 100) {
                log.info("Turning off tone");
                toneActive = false;
            }

            snd_pcm_status_t *status = 0;
            snd_pcm_status_alloca(&status);
            snd_pcm_status(playH, status);

            // State 2 = Prepared
            // State 3 = Running
            // State 4 = Underrun 
            snd_pcm_state_t currentState = snd_pcm_status_get_state(status);
            if (currentState != lastState) {
                log.info("Playback state change (%u) %d -> %d", loop, lastState, currentState);
                lastState = currentState;
            }   

            if (currentState == snd_pcm_state_t::SND_PCM_STATE_XRUN) {
                log.info("Preparing after underrun (%u)", loop);
                snd_pcm_prepare(playH);
            }

            // Delay is distance between current application frame position and sound frame position. 
            // It's positive and less than buffer size in normal situation, negative on playback underrun 
            // and greater than buffer size on capture overrun.
            unsigned delayFrames = snd_pcm_status_get_delay(status);
            if (delayFrames != lastDelayFrames) {
                log.info("Delay %u frames", delayFrames);
                lastDelayFrames = delayFrames;
            }

            if (toneActive) {

                // Make a stereo buffer (interleaved) and convert to S16_LE. We know this 
                // buffer is larger than the USB device can accept so the phase pointer will
                // be tracked carefully based on the actual write size.
                const int maxBlockSamples = 960;
                const int usbBufferSize = maxBlockSamples * 2 * 2;
                uint8_t usbBuffer[usbBufferSize];
                uint8_t* p2 = usbBuffer;
                for (unsigned i = 0; i < maxBlockSamples; i++, p2 += 4) {
                    unsigned t = (testTonePtr + i) % testToneSize;
                    // Left
                    pack_int16_le(testTone[t], p2);
                    // Right
                    pack_int16_le(testTone[t], p2 + 2);
                }

                unsigned blockSamples = 960;
                rc = snd_pcm_writei(playH, usbBuffer, blockSamples);
                if (rc < 0) {
                    if (rc == -EPIPE) {
                        log.error("Playback underrun");
                    } else if (rc == -11) {
                        log.info("Card full");
                    } else {
                        log.error("Write failed %d", rc);
                        snd_pcm_recover(playH, rc, 0); 
                        // NOTE: PROBLEM SEEN ON 11-MAY-2026, SOME ERRORS (-14) DON'T RECOVER.
                        // MAY NEED TO CLOSE/OPEN CHANNEL.
                    }
                } else if (rc > 0) {            
                    if ((unsigned)rc != lastWriteSize) {
                        log.info("Wrote %d frames", rc);
                        lastWriteSize = rc;
                    }
                    testTonePtr = (testTonePtr + rc) % testToneSize;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            loop++;
        }
    }

    return 0;
}
