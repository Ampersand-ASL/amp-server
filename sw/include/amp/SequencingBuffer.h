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

#include <cstdint>

namespace kc1fsz {

class Log;

    namespace amp {

/**
 * This abstract interface defines the output side of the Sequencing
 * Buffer. Whenever the SequencingBuffer is ready to play the next
 * frame (or request an interpolation) it will do so by calling the
 * appropriate method on this interface.
 */
template <class T> class SequencingBufferSink {
public:

    /**
     * Called to play a signalling frame.
     * @param localTime The local clock (ms, relative to the start of the call) 
     * that the signal should be processed. This is provided for reference 
     * purposes and normally would be ignored - just process the signal
     * immeditately on this method call.
     */
    virtual void playSignal(const T& frame,  uint32_t localTime) = 0;

    /**
     * Called to play a voice frame.
     * @param frame The voice content
     * @param localTime The local clock (ms, relative to the start of the call) 
     * that the voice frame should be played. This is provided for reference 
     * purposes and normally would be ignored - just process the signal
     * immeditately on this method call.
     */
    virtual void playVoice(const T& frame, uint32_t localTime) = 0;

    /**
     * Called when voice interpolation is needed. 
     * @param localTime Same as for playVoice().
     * @param duration The duration of the interpolation needed in milliseconds.
     */
    virtual void interpolateVoice(uint32_t localTime, uint32_t duration) = 0;
};

/**
 * An abstract interface of a SequencingBuffer, often called a "jitter buffer."
 * This interface defines the way that an application interacts with the buffer.
 */
template<class T> class SequencingBuffer {
public:

    /**
     * Clears all state and returns statistical parameters to initial condition.
     * This woudl typically be called at the beginging of a call.
     */
    virtual void reset() = 0;

    /**
     * Called when a full voice frame is received and ready to be processed.
     * @param remoteTime Is the time that the remote side (peer) provides to indicate
     * when it produced the frame. This is measured in milliseconds elapsed since the 
     * start of the call FROM THE REMOTE PEER'S PERSPECTIVE.
     * @param localTIme This is the time the frame was received in milliseconds 
     * elapsed since the start of the call from the local (our) perspective.
     * @return true if the message was consumed, false if it was ignored and can be 
     * discarded (i.e. all full)
     */
    virtual bool consumeVoice(Log& log, const T& payload, uint32_t remoteTime, uint32_t localTime) = 0;

    /**
     * Called when a full signal frame is received.
     * See consumeVoice()
     */
    virtual bool consumeSignal(Log& log, const T& payload, uint32_t remoteTime, uint32_t localTime) = 0;

    /**
     * Should be called periodically (precisely on the audio tick interval) to ask the 
     * buffer to produce any outgoing frames that are due at the specified localTime.
     * On each call we would expect (a) zero or more signalling frames and (b) EITHER 
     * one voice frame or one interolation request.
     * 
     * @param localTime Milliseconds elpased from the start of the call.
     * @param sink Where the frames should be sent.
     */
    virtual void playOut(Log& log, uint32_t localTime, SequencingBufferSink<T>* sink) = 0;

    /**
     * @returns true if a talkspurt is actively being played.
     */
    virtual bool inTalkspurt() const = 0;
};
    }
}
