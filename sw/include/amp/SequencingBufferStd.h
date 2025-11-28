/**
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ, All Rights Reserved
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
#pragma once

#include <cmath>

#include "kc1fsz-tools/fixedsortedlist.h"
#include "kc1fsz-tools/Log.h"

#include "amp/SequencingBuffer.h"

namespace kc1fsz {   
    namespace amp {

/**
 * The standard implementation of the SequencingBuffer. Uses
 */
template <class T> class SequencingBufferStd : public SequencingBuffer<T> {
public:

    // Any voice frames that fall outside of this range +/- are ignored
    static const int MAX_VOICE_FRAME_DIFF_MS = 5000;

    SequencingBufferStd()
    :   _buffer(_slotSpace, _ptrSpace, MAX_BUFFER_SIZE, 
        // This is the function that establishes the sort order of the frames. 
        [](const Slot& a, const Slot& b) {
            return a.compareTo(b.remoteTime);
        }) 
    { 
        reset();
    }

    void lockDelay() {
        _delayLocked = true;
    }

    void unlockDelay() {
        _delayLocked = false;
    }   

    void setDelay(unsigned ms) {
        _delay = ms;
        // Seed the adaptive buffer
        _di = _di_1 = ms;
    }

    unsigned getDelay() const {
        return _delay;
    }

    void setTalkspurtTimeoutInterval(uint32_t i) {
        _talkspurtTimeoutInteval = i;
    }

    // #### TODO: MAKE SURE WE HAVE EVERYTHING
    virtual void reset() {
        _buffer.clear();
        _overflowCount = 0;
        _lateVoiceFrameCount = 0;
        _interpolatedVoiceFrameCount = 0;
        _lastVoiceFramePlayedLocalTime = 0;
        _lastVoiceFramePlayedRemoteTime = 0;
        _inTalkspurt = false;
        _talkSpurtCount = 0;
        _talkspurtFrameCount = 0;
        _talkspurtFirstRemoteTime = 0;
        _networkDelayEstimateMs = 0;
        _remoteClockLagEstimateMs = 0;
        _voicePlayoutCount = 0;
        _voiceConsumedCount = 0;
        _di = 0;
        _di_1 = 0;
        _vi = 0;
        _vi_1 = 0;
        _maxBufferDepth = 0;
    }

    bool empty() const { return _buffer.empty(); }
    unsigned size() const { return _buffer.size(); }
    unsigned maxSize() const { return MAX_BUFFER_SIZE; }
    
    void debug() const {
        cout << "SequencingBufferStd Debug:" << endl;
        _buffer.visitAll([](const Slot& slot) {
            cout << " rt=" << slot.remoteTime << ", seq=" << slot.seq << endl;
            return true;
        });
    }

    void setNetworkDelayEstimate(int32_t m) {
        _networkDelayEstimateMs = m;
    }

    void setRemoteClockLagEstimate(int32_t m) {
        _remoteClockLagEstimateMs = m;
    }

    /**
     * Used to "extend" a 16-bit time (from a voice mini-frame) to a full
     * 32-bit representation if necessary. Assumes that both times are 
     * within the same general vicinity.
     */
    static uint32_t extendTime(uint32_t remoteTime, uint32_t localTime) {
        if ((remoteTime & 0xffff0000) == 0) {

            uint32_t r2 = remoteTime & 0x0000ffff;
            uint32_t l1 = localTime  & 0xffff0000;
            uint32_t l2 = localTime  & 0x0000ffff;

            if (l2 >= 0x8000) {
                uint32_t boundary = (l2 - 0x8000) & 0xffff;
                if (r2 < boundary) {
                    return (l1 + 0x00010000) | r2;
                } else {
                    return l1 | r2;
                }
            } else {
                uint32_t boundary = (l2 + 0x8000) & 0xffff;
                if (r2 > boundary) {
                    return (l1 - 0x00010000) | r2;
                } else {
                    return l1 | r2;
                }
            }
        }
        else {
            return remoteTime;
        }
    }

    // ----- Diagnostics -----------------------------------------------

    unsigned getLateVoiceFrameCount() const { return _lateVoiceFrameCount; }
    unsigned getInterpolatedVoiceFrameCount() const { return _interpolatedVoiceFrameCount; }
    unsigned getOverflowCount() const { return _overflowCount; }
    unsigned getMaxBufferDepth() const { return _maxBufferDepth; }
    unsigned getRemoteVoiceDelayEstimate() const { return _remoteVoiceDelayEstimateMs; }

    // ----- SequencingBuffer -------------------------------------------------

    virtual bool consumeSignal(Log& log, const T& payload, uint32_t remoteTime, uint32_t localTime) {
        return _consume(log, false, payload, remoteTime, localTime);
    }

    virtual bool consumeVoice(Log& log, const T& payload, uint32_t remoteTime, uint32_t localTime) {       
        return _consume(log, true, payload, remoteTime, localTime);
    }

    virtual void playOut(Log& log, uint32_t localTime, SequencingBufferSink<T>* sink) {     

        // For diagnostic purposes
        _maxBufferDepth = std::max(_maxBufferDepth, _buffer.size());

        // Work through the buffer chronologically. Forward on signal frames,
        // look for the start of a talk spurt, play voice frames at the 
        // right time, and discard expired voice frames.
        while (!_buffer.empty()) {
            // Signal frame
            if (!_buffer.first().voice)
                sink->playSignal(_buffer.pop().payload, localTime);
            // Voice frame
            else {
                if (!_inTalkspurt) {
                    // First frame of a call? If so, use it to set the 
                    // initial delay for the call.
                    if (_voicePlayoutCount == 0) {

                        // Check the initial delay to make sure this is in range.
                        // NOTE: It is theoretically possible to have a negative 
                        // delay in the case that the remote clock is ahead.
                        int32_t firstDelay = 
                            (int32_t)localTime - (int32_t)_buffer.first().remoteTime;
                        if (firstDelay > (int32_t)_maxDelay) {
                            log.info("Discarded old frame (%d/%d/%d)", 
                                _buffer.first().remoteTime, _buffer.first().localTime, firstDelay);
                            _lateVoiceFrameCount++;
                            _buffer.pop();
                            continue;
                        }

                        // If the delay is reasonable, take it as the starting point.
                        if (!_delayLocked) {
                            _delay = (firstDelay + (int32_t)_delaySafetyMargin);
                            log.info("New call, delay set to %d", _delay);
                        }
                    }

                    _inTalkspurt = true;
                    _talkspurtFrameCount = 0;
                    _talkspurtFirstRemoteTime = _buffer.first().remoteTime;
                }

                // Look for expired frames and discard them
                // TODO: CONSIDER EXTENDING DELAY?
                int32_t actualDelay = (int32_t)localTime - (int32_t)_buffer.first().remoteTime;
                if (actualDelay > _delay) {
                    log.info("Discarded old frame delay=%d, limit=%d", actualDelay, _delay);
                    _lateVoiceFrameCount++;
                    _buffer.pop();
                    continue;
                }
                else {
                    break;
                }
            }            
        }

        bool voicePlayed = false;

        // At this point we are in a talkspurt and the oldest frame has not expired, 
        // check to see if we should play it now or wait.
        if (!_buffer.empty()) {
            const Slot& slot = _buffer.first();
            if ((int32_t)slot.remoteTime == (int32_t)localTime - (int32_t)_delay) {
                sink->playVoice(slot.payload, localTime);
                bool startOfCall = _voicePlayoutCount == 0;
                bool startOfSpurt = _talkspurtFirstRemoteTime == slot.remoteTime;
                _voiceFramePlayed(startOfCall, startOfSpurt, slot.remoteTime, localTime);
                _lastVoiceFramePlayedLocalTime = localTime;
                _lastVoiceFramePlayedLocalTime = slot.remoteTime;
                voicePlayed = true;
                log.info("In talkspurt");
                _talkspurtFrameCount++;
                _voicePlayoutCount++;
                _buffer.pop();
            }
        }

        // If there was nothing ready to be played in the queue and we're in a talkspurt
        // then request an interpolation to fill the gap.
        if (_inTalkspurt && !voicePlayed && _talkspurtFrameCount > 0) {
            _interpolatedVoiceFrameCount++;
            sink->interpolateVoice(localTime, _voiceTickSize);
        }

        // Check to see if a talkspurt has ended
        if (_inTalkspurt && 
            _voicePlayoutCount > 0 && 
            localTime > _lastVoiceFramePlayedLocalTime + _talkspurtTimeoutInteval) {
            _inTalkspurt = false;
            log.info("Out of talkspurt");
            _talkSpurtCount++;
            _endOfTalkspurt(log);
        }
    }
    
    virtual bool inTalkspurt() const {
        return _inTalkspurt;
    }

private:

    /**
     * Each frame is tracked using one of these.
     */
    struct Slot {
        
        bool voice;
        uint32_t remoteTime;        
        uint32_t localTime;
        T payload;
        
        /**
         * Determines where this slot falls relative to the
         * timestamp/seq in chronological order.
         * 
         * @returns -1 means this slot is older, +1 means
         * this slot is newer, 0 means this slot matches.
         */
        int compareTo(uint32_t ts) const {
            if (remoteTime < ts)
                return -1;
            else if (remoteTime > ts)
                return 1;
            else 
                return 0;
        }

        bool isVoice() const { return voice; }
    };

    bool _consume(Log& log, bool isVoice, const T& payload, uint32_t remoteTime, uint32_t localTime) {
    
        if (!_buffer.hasCapacity()) {
            _overflowCount++;
            log.info("Sequencing Buffer overflow");
            return false;
        }

        _buffer.insert({ .voice=isVoice, .remoteTime=remoteTime, .localTime=localTime, 
            .payload=payload });     
            
        // Update an estimate of the remote side's audio delay. This removes the effects 
        // of the remote side's clock skey and the current network delay.
        int32_t delay = (int32_t)localTime - 
            ((int32_t)remoteTime + (int32_t)_remoteClockLagEstimateMs) -
            _networkDelayEstimateMs;
        if (delay > 0)
            _remoteVoiceDelayEstimateMs = delay;
        else 
            _remoteVoiceDelayEstimateMs = 0;

        _voiceConsumedCount++;

        return true;
    }

    void _endOfTalkspurt(Log& log) { 
        if (!_delayLocked) {
            int32_t oldDelay = _delay;
            // Update delay for next talkspurt based on the statistics,
            // making sure the delay is a multiple of 20.
            float unroundedDelay = (_di + (4.0f * _vi));
            // Round up to the nearest 20ms
            _delay = (20 * (int32_t)ceilf(unroundedDelay / 20)) + _delaySafetyMargin;
            log.info("Delay adjusted %f %d -> %d", unroundedDelay, oldDelay, _delay);
        }
    }
    
    void _voiceFramePlayed(bool startOfCall, bool startOfSpurt, 
        uint32_t frameRemoteTime, uint32_t frameLocalTime) {

        float ni = (float)frameLocalTime - (float)frameRemoteTime;

        // If this is the very first voice received for the first talkspurt
        // then use it to make an initial estimate of the delay. This can float
        // during the rest of the talkspurt.
        if (startOfCall) {
            _di = ni;
            _di_1 = ni;
            // Assume no variance at the beginning
            _vi = 0;
            _vi_1 = _vi;
        }
        else {
            // Re-estimate the variance statistics on each frame
            //
            // Please see "Adaptive Playout Mechanisms for Packetized Audio Applications
            // in Wide-Area Networks" by Ramachandran Ramjee, et. al.
            //
            // This is the classic "Algorithm 1" method
            _di = _alpha * _di_1 + (1 - _alpha) * ni;
            _di_1 = _di;
            _vi = _alpha * _vi_1 + (1 - _alpha) * fabs(_di - ni);
            _vi_1 = _vi;
        }
    }

    // A 64-entry buffer provides room to track 1 second of audio
    // plus some extra for control frames that may be interspersed.
    const static unsigned MAX_BUFFER_SIZE = 64;
    Slot _slotSpace[MAX_BUFFER_SIZE];
    unsigned _ptrSpace[MAX_BUFFER_SIZE];
    fixedsortedlist<Slot> _buffer;

    // Used for detecting the end of a talkspurt
    uint32_t _lastVoiceFramePlayedLocalTime = 0;

    bool _inTalkspurt = false;
    unsigned _talkspurtFrameCount = 0;
    uint32_t _talkspurtTimeoutInteval = 60;
    bool _delayLocked = false;
    int32_t _delay = 0;
    // This is locked in at the start of the talkspurt. 
    uint32_t _talkspurtFirstRemoteTime;
    uint32_t _lastVoiceFramePlayedRemoteTime = 0;

    // Used to estimate delay and delay variance
    float _di_1 = 0;
    float _di = 0;
    float _vi = 0;
    float _vi_1 = 0; 

    // ------ Configuration Constants ----------------------------------------

    const int32_t _maxDelay = 1000;

    const uint32_t _voiceTickSize = 20;

    // This amount is always added any time we update the delay. This provides
    // a small amount of buffer over what the calculated delay says we should 
    // use in case of additional timing problems.
    // MUST BE A MULTIPLE OF _voiceTickSize
    const unsigned _delaySafetyMargin = _voiceTickSize * 3;

    // For Algorithm 1
    const float _alpha = 0.998002;

    // ----- Diagnostic/Metrics Stuff ----------------------------------------

    unsigned _overflowCount = 0;
    unsigned _lateVoiceFrameCount = 0;
    unsigned _interpolatedVoiceFrameCount = 0;
    unsigned _voicePlayoutCount = 0;
    unsigned _voiceConsumedCount = 0;

    // The number of talkspurts since reset. This is incremented at the end of 
    // each talkspurt. Importantly, it will be zero for the duration of the 
    // first talkspurt.
    unsigned _talkSpurtCount = 0;
    int32_t _networkDelayEstimateMs = 0;
    int32_t _remoteClockLagEstimateMs = 0;
    int32_t _remoteVoiceDelayEstimateMs = 0;
    unsigned _maxBufferDepth = 0;
};    

    }
}


