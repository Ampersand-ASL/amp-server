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
        _di_1 = ms;
    }

    unsigned getDelay() const {
        return _delay;
    }

    void setTalkspurtTimeoutInterval(uint32_t i) {
        _talkSpurtTimeoutInteval = i;
    }

    // #### TODO: MAKE SURE WE HAVE EVERYTHING
    virtual void reset() {
        _buffer.clear();
        _overflowCount = 0;
        _lateVoiceFrameCount = 0;
        _interpolatedVoiceFrameCount = 0;
        _lastVoiceFramePlayedTime = 0;
        _voiceFramesPlayedInThisTick = 0;
        _voicePlayoutCount = 0;
        _inTalkSpurt = false;
        _talkSpurtCount = 0;
        _networkDelayEstimateMs = 0;
        _remoteClockLagEstimateMs = 0;
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
        // This is a heuristic that assumes (conservatively) that a good guess 
        // for the initial delay margin that should be applied at the start of 
        // a new call is 25% of the estimated network delay.
        _initialDelayMargin = m / 4;
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

    static void runUnitTests() {
        // remote, local
        assert(extendTime(0x0001, 0x00020001) == 0x00020001);
        assert(extendTime(0x0001, 0x0001ffff) == 0x00020001);
        assert(extendTime(0xffff, 0x00020001) == 0x0001ffff);
        assert(extendTime(0xffff, 0x0002fffe) == 0x0002ffff);
    }

    // ----- SequencingBuffer -------------------------------------------------

    virtual bool consumeSignal(Log& log, const T& payload, uint32_t remoteTime, uint32_t localTime) {
        return _consume(log, false, payload, remoteTime, localTime);
    }

    virtual bool consumeVoice(Log& log, const T& payload, uint32_t remoteTime, uint32_t localTime) {       
        return _consume(log, true, payload, remoteTime, localTime);
    }

    /*
    Variables

    bool _inTalkspurt = false;
    // Keeps track of how many 20ms ticks have happened
    // in this talkspurt.
    unsigned _talkspurtTickCount = 0;
    uint32_t _talkspurtFirstRemoteTime;
    uint32_t _delay;


    Logic:
    1. Keep a flag that indicates when we are in a talkspurt. This
    flag will be set when we see the first packet and will be 
    cleared after inactivity timeout or on an explicit UNKEY.
    2. Lock in the first remote timestamp of a talkspurt 
    called _talkspurtFirstRemoteTime.
    3. All subsequent voice frames are expected at 
    _talkspurtRemoteFirstTime + _talkspurtTickCount * 20ms.
    4. On the first frame we should set _delay so that it is at
    least large enough to capture the first frame. It might be
    good to set the delay out 20 or 40ms longer to provide a bit of 
    margin.
    5. At each tick the targetRemoteTime = localTime - _delay. 
    6. Look at the oldest frame in the jitter buffer.
    7. Is the oldest frame remoteTime = targetRemoteTime? If so, play it,
    this is the happy path.
    8. If the oldest frame remoteTime is > targetRemoteTime then either 
    (a) if we have not played the first frame in the talkspurt yet,
    then do nothing. This is determined by targetRemoteTime < 
    _talkspurtFirstRemoteTime.
    or
    (b) if we have played the first frame in the talkspurt then this
    must be a gap created by a missing frame, interpolate the tick.
    And then consider _delay extension by one tick. This will cause
    us to look for exactly the same frame on the next tick.
    9. If the oldest frame is < targetRemoteTime then we discard
    it (this must be a late arriving frame from a tick that we skipped
    previouslt).
    */





    virtual void playOut(Log& log, uint32_t localTime, SequencingBufferSink<T>* sink) {     

        // For diagnostic purposes
        _maxBufferDepth = std::max(_maxBufferDepth, _buffer.size());

        // TODO: NEED TO CHANGE THIS!
        _voiceFramesPlayedInThisTick = 0;

        // Visit and then clear out any messages that were received
        // in (or before) the current window.

        // We switch into signed since the window may extend into 
        // negative time at the very start of a call.
        const int32_t playOutWindowEnd = (int32_t)localTime - (int32_t)_delay;
        const int32_t playOutWindowStart = playOutWindowEnd - (int32_t)_voiceTickSize;

        // First clear out any expired voice frames from the stream keeping track 
        // of statistics along the way.
        unsigned lateVoiceFrames = 0;

        _buffer.visitIfAndRemove(
            // Visitor
            [playOutWindowStart, playOutWindowEnd, &lateVoiceFrames, &log, delay=_delay]
            (const Slot& slot) {
                lateVoiceFrames++;
                log.info("Discarded voice %d [%d->%d) %d",
                    (int32_t)slot.remoteTime, 
                    playOutWindowStart, playOutWindowEnd,
                    (int32_t)delay);
                return true;
            },
            // Predicate
            [playOutWindowStart]
            (const Slot& slot) {
                // Comparison needs to be signed
                return slot.isVoice() && (int32_t)slot.remoteTime < playOutWindowStart;
            }
        );

        if (lateVoiceFrames) {
            _lateVoiceFrameCount += lateVoiceFrames;
        }

        // Playout the frames relevant to this window

        _buffer.visitIfAndRemove(
            // Visitor - do the actual playout for anything that passes
            // the predicate below.
            [localTime, sink, context=this]
            (const Slot& slot) {
                if (slot.isVoice()) {

                    bool startOfSpurt = !context->_inTalkSpurt;
                    context->_inTalkSpurt = true;
                    context->_voiceFramesPlayedInThisTick++;
                    context->_lastVoiceFramePlayedTime = localTime;
                    sink->playVoice(slot.payload, localTime);

                    if (startOfSpurt)
                        context->_startOfTalkspurt();
                    context->_voiceFramePlayed(startOfSpurt, slot.remoteTime, slot.localTime, localTime);
                } 
                else {
                    sink->playSignal(slot.payload, localTime);
                }
                return true;
            },
            // Predicate - find relevant frames to be played out
            [playOutWindowEnd,
                // Careful, we need a live copy of this counter
                // since the visitor might be updating as we go
                // through the window.
                &previousVoice=_voiceFramesPlayedInThisTick]
            (const Slot& slot) {
                // Anything in the window?
                // Use signed comparison because of some negative times at the 
                // start of a call.
                if ((int32_t)slot.remoteTime < playOutWindowEnd) {
                    // Voice frames need special treatment to avoid
                    // multiple frames in the same tick.
                    if (slot.isVoice()) {
                        if (previousVoice > 0) {
                            // #### TODO: Stats for this case
                            return false;
                        }
                        else return true;
                    }
                    else return true;
                }
                else return false;
            }
        );

        // Check to see if a talkspurt has ended
        if (_inTalkSpurt && localTime > 
            _lastVoiceFramePlayedTime + _talkSpurtTimeoutInteval) {
            _inTalkSpurt = false;
            _talkSpurtCount++;
            _endOfTalkspurt();
        }

        // If we get here and (a) we're in a talkspurt and (b) we're at the end of a 
        // voice period and (c) no voice frames have been produced then we need to 
        // request an interpolation.
        if (_inTalkSpurt && _voiceFramesPlayedInThisTick == 0) {
            _interpolatedVoiceFrameCount++;
            sink->interpolateVoice(localTime, _voiceTickSize);
        }
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

        // The pure mapping offset between local and remote time
        int32_t ni = (int32_t)localTime - (int32_t)remoteTime;
        if (ni < 0) {
            ni = 0;
        }

        // Make sure we immediately discard any frames that are way out of range
        if (abs(ni) > MAX_VOICE_FRAME_DIFF_MS) {
            log.info("Voice frame out of range, dropped");
            return false;
        }

        _buffer.insert({ .voice=isVoice, .remoteTime=remoteTime, .localTime=localTime, 
            .payload=payload });     
            
        // If this is the very first voice received for the first talkspurt
        // then use it to make an initial estimate of the delay. This can float
        // during the rest of the talkspurt.
        if (_voiceConsumedCount == 0) {
            _di = ni;
            _di_1 = ni;
            // Since we have no history from which to construct a network 
            // variance term, we use a fixed guess of "initial margin" and allow
            // the adaptive algorithm to fix this over time.
            _vi = _initialDelayMargin / 4.0f;
            _vi_1 = _vi;
            if (!_delayLocked) {
                // Note here that we're giving ourselves an extra frame margin
                _delay = std::max(_di + (4.0f * _vi), _delayMin);
                _delay += _voiceTickSize;
                log.info("First frame, delay %d", _delay);
            }
        }
        // For subsequent frames look to see whether the delay needs to be
        // extended outward to accommodate this frame.
        else {
            if (!_delayLocked) {
                // Current window parameters
                const int32_t playOutWindowEnd = (int32_t)localTime - (int32_t)_delay;
                const int32_t playOutWindowStart = playOutWindowEnd - (int32_t)_voiceTickSize;
                // If the new frame is arriving too late to fall in the window 
                // then extend the delay to capture it
                if ((int32_t)remoteTime < playOutWindowStart && ni > (int32_t)_delay) {
                    uint32_t oldDelay = _delay;
                    // Note here that we're giving ourselves an extra frame margin
                    _delay = ni;
                    _delay += _voiceTickSize;
                    log.info("Delay %d->%d", oldDelay, _delay);
                }
            }
        }

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

    void _startOfTalkspurt() { 
    }

    void _endOfTalkspurt() { 
        // Update delay for next talkspurt based on the statistics
        if (!_delayLocked)
            _delay = std::max(_di + (4.0f * _vi), _delayMin);
    }
    
    void _voiceFramePlayed(bool startOfSpurt, uint32_t frameRemoteTime, uint32_t frameLocalTime,
        uint32_t) {
        // Re-estimate the variance statistics on each frame
        //
        // Please see "Adaptive Playout Mechanisms for Packetized Audio Applications
        // in Wide-Area Networks" by Ramachandran Ramjee, et. al.
        //
        // This is the classic "Algorithm 1" method
        float ni = (float)frameLocalTime - (float)frameRemoteTime;
        _di = _alpha * _di_1 + (1 - _alpha) * ni;
        _di_1 = _di;
        _vi = _alpha * _vi_1 + (1 - _alpha) * fabs(_di - ni);
        _vi_1 = _vi;
    }

    // A 64-entry buffer provides room to track 1 second of audio
    // plus some extra for control frames that may be interspersed.
    const static unsigned MAX_BUFFER_SIZE = 64;
    Slot _slotSpace[MAX_BUFFER_SIZE];
    unsigned _ptrSpace[MAX_BUFFER_SIZE];
    fixedsortedlist<Slot> _buffer;

    uint32_t _voiceTickSize = 20;
    uint32_t _talkSpurtTimeoutInteval = 80;
    bool _delayLocked = false;
    unsigned _delay = 0;
    uint32_t _lastVoiceFramePlayedTime = 0;
    unsigned _voiceFramesPlayedInThisTick = 0;
    unsigned _overflowCount = 0;
    unsigned _lateVoiceFrameCount = 0;
    unsigned _interpolatedVoiceFrameCount = 0;
    unsigned _voicePlayoutCount = 0;
    unsigned _voiceConsumedCount = 0;
    bool _inTalkSpurt = false;
    // The number of talkspurts since reset. This is incremented at the end of 
    // each talkspurt. Importantly, it will be zero for the duration of the 
    // first talkspurt.
    unsigned _talkSpurtCount = 0;
    int32_t _networkDelayEstimateMs = 0;
    int32_t _remoteClockLagEstimateMs = 0;
    int32_t _remoteVoiceDelayEstimateMs = 0;
    unsigned _maxBufferDepth = 0;

    // This is a guess of the margin that should be assumed for the very 
    // first delay update for a new call. Once the call gets going the 
    // real variance calculation will take over. This number may possibly
    // be updated by the ping latency testing before the guess is used.
    unsigned _initialDelayMargin = 10;

    // Used to estimate delay and delay variance
    float _di_1 = 0;
    float _di = 0;
    float _vi = 0;
    float _vi_1 = 0; 

    // For Algorithm 1
    float _alpha = 0.998002;
    // The minimum delay possible.  It's good to have a bit or margin to take into account 
    // some inprecision in the local timing mechanisms.
    float _delayMin = 20;
};    

    }
}


