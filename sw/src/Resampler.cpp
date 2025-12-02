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
#include <cstring>
#include <cassert>

#include "amp/Resampler.h"

namespace kc1fsz {
    namespace amp {

// REMEMBER: These are in reverse order but since they are symmetrical
// and an odd number this doesn't matter.
const int16_t Resampler::F1_COEFFS[] = { 103, 136, 148, 74, -113, -395, -694,
    -881, -801, -331, 573, 1836, 3265, 4589, 5525, 5864, 5525,
    4589, 3265, 1836, 573, -331, -801, -881, -694, -395, -113,
    74, 148, 136, 103 };

// REMEMBER: These are in reverse order but since they are symmetrical
// and an odd number this doesn't matter.
const int16_t Resampler::F2_COEFFS[] = { 103, 136, 148, 74, -113, -395, -694,
    -881, -801, -331, 573, 1836, 3265, 4589, 5525, 5864, 5525,
    4589, 3265, 1836, 573, -331, -801, -881, -694, -395, -113,
    74, 148, 136, 103 };

void Resampler::setRates(unsigned inRate, unsigned outRate) {
    _inRate = inRate;
    _outRate = outRate;
    reset();
}

void Resampler::reset() {
    assert(_inRate != 0 && _outRate != 0);
    if (_inRate == _outRate) {
        // No filter needed
    } else if (_inRate == 8000 && _outRate == 48000) {
        arm_fir_init_q15(&_lpfFilter, F1_TAPS, F1_COEFFS, _lpfState, BLOCK_SIZE_48K);
    } else if (_inRate == 48000 && _outRate == 8000) {
        arm_fir_init_q15(&_lpfFilter, F2_TAPS, F2_COEFFS, _lpfState, BLOCK_SIZE_48K);
    } else {
        assert(false);
    }
}

unsigned Resampler::getInBlockSize() const {
    return _getBlockSize(_inRate);
}

unsigned Resampler::getOutBlockSize() const {
    return _getBlockSize(_outRate);
}

unsigned Resampler::_getBlockSize(unsigned rate) const {
    if (rate == 8000)
        return BLOCK_SIZE_8K;
    else if (rate == 48000)
        return BLOCK_SIZE_48K;
    else 
        assert(false);
}

void Resampler::resample(const int16_t* inBlock, unsigned inSize, 
    int16_t* outBlock, unsigned outSize) {
    assert(_inRate != 0 && _outRate != 0);
    if (_inRate == _outRate) {
        assert(inSize == outSize);
        memcpy(outBlock, inBlock, sizeof(int16_t) * getInBlockSize());
    }
    else if (_inRate == 8000 && _outRate == 48000) {
        assert(inSize == BLOCK_SIZE_8K);
        assert(outSize == BLOCK_SIZE_48K);
        // Perform the upsampling to 48k.
        int16_t pcm48k_1[BLOCK_SIZE_48K];
        int16_t* p1 = pcm48k_1;
        const int16_t* p0 = inBlock;
        for (unsigned i = 0; i < BLOCK_SIZE_8K; i++, p0++)
            for (unsigned j = 0; j < 6; j++)
                *(p1++) = *p0;
        // Apply the LPF anti-aliasing filter
        arm_fir_q15(&_lpfFilter, pcm48k_1, outBlock, BLOCK_SIZE_48K);
    }
    else if (_inRate == 48000 && _outRate == 8000) {
        assert(inSize == BLOCK_SIZE_48K);
        assert(outSize == BLOCK_SIZE_8K);
        // Decimate from 48k to 8k
        // Apply a LPF to the block because we are decimating.
        // TODO: Use the more efficient decimation filter.
        int16_t pcm48k_1[BLOCK_SIZE_48K];
        arm_fir_q15(&_lpfFilter, inBlock, pcm48k_1, BLOCK_SIZE_48K);
        const int16_t* srcPtr = pcm48k_1;
        for (unsigned i = 0; i < BLOCK_SIZE_8K; i++, srcPtr += 6)
            outBlock[i] = *srcPtr;
    }
    else {
        assert(false);
    }
}
    }
}
