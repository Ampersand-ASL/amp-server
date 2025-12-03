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

const int16_t Resampler::F16_COEFFS[] = {
    -3915, -295, 1127, -1312, 481, 531, -508, -720, 1840, -1315, -770, 2263, -691, -4236, 9813, 20508, 9813, -4236, -691, 2263, -770, -1315, 1840, -720, -508, 531, 481, -1312, 1127, -295, -3915    
};

void Resampler::setRates(unsigned inRate, unsigned outRate) {

    reset();

    _inRate = inRate;
    _outRate = outRate;

    if (_inRate == _outRate) {
        // No filter needed
    } else if (_inRate == 8000 && _outRate == 48000) {
        arm_fir_init_q15(&_lpfFilter, F1_TAPS, F1_COEFFS, _lpfState, BLOCK_SIZE_48K);
    } else if (_inRate == 48000 && _outRate == 8000) {
        arm_fir_init_q15(&_lpfFilter, F2_TAPS, F2_COEFFS, _lpfState, BLOCK_SIZE_48K);
    } else if (_inRate == 16000 && _outRate == 48000) {
        arm_fir_init_q15(&_lpfFilter, F16_TAPS, F16_COEFFS, _lpfState, BLOCK_SIZE_48K);
    } else if (_inRate == 48000 && _outRate == 16000) {
        arm_fir_init_q15(&_lpfFilter, F16_TAPS, F16_COEFFS, _lpfState, BLOCK_SIZE_48K);
    } else {
        assert(false);
    }
}

void Resampler::reset() {
    memset(_lpfState, 0, sizeof(_lpfState));
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
    if (rate == 16000)
        return BLOCK_SIZE_16K;
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
    else if (_inRate == 16000 && _outRate == 48000) {
        assert(inSize == BLOCK_SIZE_16K);
        assert(outSize == BLOCK_SIZE_48K);
        // Perform the upsampling to 48k.
        int16_t pcm48k_1[BLOCK_SIZE_48K];
        int16_t* p1 = pcm48k_1;
        const int16_t* p0 = inBlock;
        for (unsigned i = 0; i < BLOCK_SIZE_16K; i++, p0++)
            for (unsigned j = 0; j < 3; j++)
                *(p1++) = *p0;
        // Apply the LPF anti-aliasing filter
        arm_fir_q15(&_lpfFilter, pcm48k_1, outBlock, BLOCK_SIZE_48K);
    }
    else if (_inRate == 48000 && _outRate == 16000) {
        assert(inSize == BLOCK_SIZE_48K);
        assert(outSize == BLOCK_SIZE_16K);
        // Decimate from 48k 
        // Apply a LPF to the block because we are decimating.
        // TODO: Use the more efficient decimation filter.
        int16_t pcm48k_1[BLOCK_SIZE_48K];
        arm_fir_q15(&_lpfFilter, inBlock, pcm48k_1, BLOCK_SIZE_48K);
        const int16_t* srcPtr = pcm48k_1;
        for (unsigned i = 0; i < BLOCK_SIZE_16K; i++, srcPtr += 3)
            outBlock[i] = *srcPtr;
    }
    else {
        assert(false);
    }
}
    }
}
