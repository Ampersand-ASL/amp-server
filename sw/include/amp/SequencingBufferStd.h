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
 * Adaptive Jitter Bufer.
 * 
 * The method I've settled on at the moment is called "Ramjee Algorithm 1" after a 
 * paper by Ramjee, Kurose, Towsley, and Schulzrinne called "Adaptive Playout 
 * Mechanisms for Packetized Audio Applications in Wide-Area Networks." This
 * is an IEEE paper behind the paywell.
 */
template <class T> class SequencingBufferStd : public SequencingBuffer<T> {
public:

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

    void setInitialMargin(int32_t ms) {
        _initialMargin = ms;
        // Seed the adaptive buffer
        _di = _di_1 = ms;
        _vi = _vi_1 = 0;
    }

    unsigned getDelay() const {
        //return _delay;
        return 0;
    }

    void setTalkspurtTimeoutInterval(uint32_t i) {
        _talkspurtTimeoutInteval = i;
    }

    // #### TODO: MAKE SURE WE HAVE EVERYTHING
    virtual void reset() {
        _buffer.clear();
        // Diagnostics
        _maxBufferDepth = 0;
        _overflowCount = 0;
        _lateVoiceFrameCount = 0;
        _interpolatedVoiceFrameCount = 0;
        // Tracking
        _lastPlayedLocal = 0;
        _lastPlayedOrigin = 0;
        _originCursor = 0;
        _inTalkspurt = false;
        _talkSpurtCount = 0;
        _talkspurtFrameCount = 0;
        _talkspurtFirstOrigin = 0;
        _voicePlayoutCount = 0;
        _voiceConsumedCount = 0;
        _di = 0;
        _di_1 = 0;
        _vi = 0;
        _vi_1 = 0;
        _idealDelay = 0;
        _worstMargin = 0;    
        _totalMargin = 0;    
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

    static int32_t roundUpToTick(int32_t v, int32_t tick) {
        float a = ceilf((float)v / (float)tick);
        return a * (float)tick;
    }

    static int32_t roundToTick(int32_t v, int32_t tick) {
        float a = round((float)v / (float)tick);
        return a * (float)tick;
    }

    // ----- Diagnostics -----------------------------------------------

    unsigned getLateVoiceFrameCount() const { return _lateVoiceFrameCount; }
    unsigned getInterpolatedVoiceFrameCount() const { return _interpolatedVoiceFrameCount; }
    unsigned getOverflowCount() const { return _overflowCount; }
    unsigned getMaxBufferDepth() const { return _maxBufferDepth; }

    // ----- SequencingBuffer -------------------------------------------------

    virtual bool consumeSignal(Log& log, const T& payload, uint32_t remoteTime, uint32_t localTime) {
        return _consume(log, false, payload, remoteTime, localTime);
    }

    virtual bool consumeVoice(Log& log, const T& payload, uint32_t remoteTime, uint32_t localTime) {       
        return _consume(log, true, payload, remoteTime, localTime);
    }

    /**
     * @param localTime The call-relative local time, but must be on _voiceTickSize
     * boundaries. For example, if tick=20ms we'd expect to see localTime : 100, 120, 140,
     * 160, 180, 200, 220, 240, ...
     */
    virtual void playOut(Log& log, uint32_t localTime, SequencingBufferSink<T>* sink) {     

        bool voiceFramePlayed = false;

        // For diagnostic purposes
        _maxBufferDepth = std::max(_maxBufferDepth, _buffer.size());

        // Work through the buffer chronologically. Forward on signal frames,
        // look for the start of a talk spurt, play voice frames at the 
        // right time, and discard expired voice frames.
        while (!_buffer.empty()) {

            // Signal frames are passed along immediately
            if (!_buffer.first().voice)
                sink->playSignal(_buffer.pop().payload, localTime);

            // Voice frame
            else {
                const Slot& slot = _buffer.first();
                const int32_t oldOriginCursor = _originCursor;

                // Old voice frames (out of order or repeats) are discarded immediately
                if (slot.remoteTime <= _lastPlayedOrigin) {
                    log.info("Discarded OOO frame (%d <= %d)", slot.remoteTime, _lastPlayedOrigin);
                    _lateVoiceFrameCount++;
                    _buffer.pop();
                    // NOTICE: We're in a loop so we get another shot at it,
                    continue;
                }

                // First frame of the talkpsurt? 
                if (!_inTalkspurt) {
                    
                    // Set the starting delay for a new call uses a configured initial margin.
                    // This will be refined as we gather statistic on the actual connection.
                    if (_voicePlayoutCount == 0) {
                        _originCursor = roundToTick(slot.remoteTime - _initialMargin,
                            _voiceTickSize);
                    } 
                    // After the call is up and running we use the adaptive algorithm to track
                    // the delay between arrival and playback.
                    else {
                        // We only get to change the delay at the start of a talkspurt.
                        // There are a few cases:
                        // 1. The ideal delay wants us to slow down. In that case we shift
                        //    the cursor backwards, being careful not to go back any further
                        //    than the last frame played.
                        //
                        // 2. The ideal dealy wants us to speed up. In that case we shift
                        //    the cursor forward, being careful not to pass the next availble
                        //    frame in the buffer.
                        // 
                        int32_t idealOriginCursor = roundToTick(
                            (int32_t)localTime - (int32_t)_idealDelay, _voiceTickSize);

                        if (idealOriginCursor < _originCursor)
                            _originCursor = std::max(idealOriginCursor, (int32_t)_lastPlayedOrigin);
                        else if (idealOriginCursor > _originCursor)
                            _originCursor = std::min(idealOriginCursor, (int32_t)slot.remoteTime);

                        if (_originCursor > oldOriginCursor)
                            log.info("Start TS, moving cursor forward %u -> %u", 
                                oldOriginCursor, _originCursor);
                        else if (_originCursor < oldOriginCursor)
                            log.info("Start TS, moving cursor backward %u <- %u", 
                                _originCursor, oldOriginCursor);
                        else 
                            log.info("Start TS, No cursor movement");
                    }

                    _inTalkspurt = true;
                    _talkspurtFrameCount = 0;
                    _talkspurtFirstOrigin = slot.remoteTime;
                    _lastPlayedOrigin = 0;
                    _lastPlayedLocal = 0;
                }

                // If we get an expired frame then either slow down a bit to pick it up
                // or discard it and move on.
                if ((int32_t)slot.remoteTime < _originCursor) {
                    // Is it OK to slow down for this one?
                    if (_originCursor - (int32_t)slot.remoteTime <= _midTsAdjustMax) {
                        log.info("Mid TS, adjusting (%d < %d)", slot.remoteTime, _originCursor);
                        // NOTE: It has already been establishe that slot.remoteTime is larger
                        // that _lastPlayedOrigin so there is no risk in moving back to this
                        // point in the stream.
                        _originCursor = slot.remoteTime;
                    } 
                    // Too late, move on
                    else {
                        log.info("Mid TS, discarded frame (%d < %d)", slot.remoteTime, _originCursor);
                        _lateVoiceFrameCount++;
                        _buffer.pop();
                    }
                    // NOTICE: We're in a loop so we get another shot at it,
                }
                // If we got the frame we are waiting for then play it
                else if ((int32_t)slot.remoteTime == _originCursor) {
                    
                    sink->playVoice(slot.payload, localTime);

                    voiceFramePlayed = true;
                    _lastPlayedLocal = localTime;
                    _lastPlayedOrigin = slot.remoteTime;
                    _voicePlayoutCount++;

                    bool startOfSpurt = _talkspurtFirstOrigin == slot.remoteTime;

                    // Keep margin tracking up to date
                    int32_t margin = (int32_t)localTime - (int32_t)slot.localTime;
                    if (startOfSpurt) {
                        _worstMargin = margin;
                        _totalMargin = margin;
                        _talkspurtFrameCount = 1;
                    } else {
                        if (margin < _worstMargin) 
                            _worstMargin = margin;
                        _totalMargin += margin;
                        _talkspurtFrameCount++;
                    }

                    _buffer.pop();

                    // We can only play one frame per tick, so break out of the loop
                    break;
                }
                // Otherwise the next voice is in the future so there's nothing more to do
                // in this tick but wait.
                else {
                    break;
                }
            }            
        }

        // Things to check while the talkspurt is running
        if (_inTalkspurt && _talkspurtFrameCount > 0) {

            // If no voice was generated on this tick (for whatever reason)
            // then request an interpolation.
            if (!voiceFramePlayed) {
                sink->interpolateVoice(localTime, _voiceTickSize);
                _interpolatedVoiceFrameCount++;
                log.info("Interpolated %u", _originCursor);
            }

            // Has the talkspurt has timed out yet?
            if (localTime > _lastPlayedLocal + _talkspurtTimeoutInteval) {
                _inTalkspurt = false;
                _talkSpurtCount++;
                int32_t avgMargin = (_talkspurtFrameCount != 0) ? 
                    _totalMargin / _talkspurtFrameCount : 0;
                log.info("End TS, avgM: %d, shortM: %d", avgMargin, _worstMargin); 
            }
        }

        // Always move the expectation forward one click to keep in sync with 
        // the clock moving forward on the remote side.
        _originCursor += _voiceTickSize;
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
         * @returns Call time offset between local arrival time and
         * remote generation time. Theoretically could be negative
         * if start times are significantly different. 
         */
        int32_t offset() const { 
            return (int32_t)localTime - (int32_t)remoteTime; 
        }

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

    bool _consume(Log& log, bool isVoice, const T& payload, uint32_t origTime, uint32_t rxTime) {
    
        if (!_buffer.hasCapacity()) {
            _overflowCount++;
            log.info("Sequencing Buffer overflow");
            return false;
        }

        _buffer.insert({ .voice=isVoice, .remoteTime=origTime, .localTime=rxTime, 
            .payload=payload });     

        if (isVoice) {
            bool startOfCall = _voiceConsumedCount == 0;
            _voiceConsumedCount++;
            // Use the frame information to keep the delay estimate up to date
            _updateDelayTarget(log, startOfCall, rxTime, origTime);
        }

        return true;
    }

    // This should be called on each voice frame arrival so that we have the 
    // most timely information about the network conditions.
    void _updateDelayTarget(Log& log, bool startOfCall, 
        uint32_t frameRxTime, uint32_t frameOrigTime) {

        // Calculate the flight time of this frame
        float ni = ((float)frameRxTime - (float)frameOrigTime);

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
        // Re-estimate the variance statistics on each frame
        else {
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

        // This is the current estimate of the ideal delay
        _idealDelay = _di + _beta * _vi;
    }

    // ------ Configuration Constants ----------------------------------------

    // The size of an audio tick in milliseconds
    const uint32_t _voiceTickSize = 20;
    // This is the most the playback cursor can be adjusted to pick up a 
    // late frame inside of a talkspurt
    const int32_t _midTsAdjustMax = 40;
    // Constants for Ramjee Algorithm 1
    const float _alpha = 0.998002f;
    const float _beta = 5.0f;
    // The number of ms of silence before we delcare a talkspurt ended.
    const uint32_t _talkspurtTimeoutInteval = 60;   

    // A 64-entry buffer provides room to track 1 second of audio
    // plus some extra for control frames that may be interspersed.
    const static unsigned MAX_BUFFER_SIZE = 64;
    Slot _slotSpace[MAX_BUFFER_SIZE];
    unsigned _ptrSpace[MAX_BUFFER_SIZE];
    fixedsortedlist<Slot> _buffer;

    // This is the important variable. This always points to the next
    // origin time to be played.
    int32_t _originCursor = 0;
    uint32_t _talkspurtFirstOrigin = 0;
    uint32_t _lastPlayedOrigin = 0;
    // Used for detecting the end of a talkspurt
    uint32_t _lastPlayedLocal = 0;

    bool _inTalkspurt = false;
    unsigned _talkspurtFrameCount = 0;

    bool _delayLocked = false;

    // Used to estimate delay and delay variance
    float _di_1 = 0;
    float _di = 0;
    float _vi = 0;
    float _vi_1 = 0; 
    float _idealDelay = 0;
    // Starting estimate of margin
    // MUST BE A MULTIPLE OF _voiceTickSize
    unsigned _initialMargin = _voiceTickSize * 3;

    // ----- Diagnostic/Metrics Stuff ----------------------------------------

    unsigned _overflowCount = 0;
    unsigned _lateVoiceFrameCount = 0;
    unsigned _interpolatedVoiceFrameCount = 0;
    unsigned _voicePlayoutCount = 0;
    unsigned _voiceConsumedCount = 0;
    int32_t _worstMargin = 0;
    int32_t _totalMargin = 0;

    // The number of talkspurts since reset. This is incremented at the end of 
    // each talkspurt. Importantly, it will be zero for the duration of the 
    // first talkspurt.
    unsigned _talkSpurtCount = 0;
    unsigned _maxBufferDepth = 0;
};    

    }
}


