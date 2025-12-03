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

// NOTE: This may be the real ARM library or a mock, depending on the
// platfom that we are building for.
#include <arm_math.h>

namespace kc1fsz {
    namespace amp {

/**
 * Used to convert between various PCM16 sampling rates. The 
 * most efficient/effective method will be chosen for each
 * combination.
 * 
 * NOTE: You shouldn't share resamplers between audio streams
 * since there is state maintained inside of the filter that improves
 * the quality of the transitions between each block.
 */
class Resampler {
public:

    static const unsigned BLOCK_SIZE_8K = 160;
    static const unsigned BLOCK_SIZE_16K = 160 * 2;
    static const unsigned BLOCK_SIZE_48K = 160 * 6;
    static const unsigned BLOCK_PERIOD_MS = 20;
    static const unsigned MAX_TAPS = 31;

    /** 
     * Resets internal state without changing the sample rates.
     */
    void reset();
    void setRates(unsigned inRate, unsigned outRate);

    /**
     * Resamples a 20ms block of audio. The sizes of these
     * blocks is implicit in the sample rate selected, so 
     * use caution to make sure that the blocks are the 
     * correct length.
     */
    void resample(const int16_t* inBlock, unsigned inSize, int16_t* outBlock, unsigned outSize);

    unsigned getInBlockSize() const;
    unsigned getOutBlockSize() const;

public:

    // Filter 1 is the LPF used for up-sampling from 8K to 
    // 48K. This runs at 48K.
    static const unsigned F1_TAPS = 31;
    // REMEMBER: These are in reverse order but since they are symmetrical
    // and an odd number this doesn't matter.
    static const int16_t F1_COEFFS[F1_TAPS];

    // Filter 2 is the LPF used for down-sampling from 48K
    // to 8K. This runs at 48K.
    static const unsigned F2_TAPS = 31;
    // REMEMBER: These are in reverse order but since they are symmetrical
    // and an odd number this doesn't matter.
    static const int16_t F2_COEFFS[F2_TAPS];

    // LPF used for down-sampling from 48K to 16K
    static const unsigned F16_TAPS = 31;
    // REMEMBER: These are in reverse order but since they are symmetrical
    // and an odd number this doesn't matter.
    static const int16_t F16_COEFFS[F16_TAPS];

private:

    unsigned _getBlockSize(unsigned rate) const;

    unsigned _inRate = 0, _outRate = 0;

    // Space for the largest possible filter
    arm_fir_instance_q15 _lpfFilter;
    int16_t _lpfState[MAX_TAPS + BLOCK_SIZE_48K - 1];
};
    }
}
