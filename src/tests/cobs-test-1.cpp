#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>
#include <cstring>

#include "kc1fsz-tools/Common.h"
#include "cobs.h"

using namespace std;
using namespace kc1fsz;

int main(int, const char**) {
    // A demonstration of what happens when you assume the COBS
    // encoding is a fixed length. It's not because the encoding
    // size depends on the message content.
    {
        uint8_t inmsg[322];
        for (unsigned i = 0; i < 322; i++) {
            inmsg[i] = (i % 2 == 0) ? 0x18 : 0x00;
        }
        uint8_t outmsg[324];
        cobs_encode_result re = cobs_encode(outmsg, 324, inmsg, 322);
        //prettyHexDump(outmsg, 302, cout);
        assert(re.status == COBS_ENCODE_OK);
        // It's actually 323
        assert(re.out_len <= 324);
        uint8_t outmsg2[322];
        // This will be an error because the encoded buffer is only 323!
        cobs_decode_result rd = cobs_decode(outmsg2, size(outmsg2), outmsg, 324);
        assert(rd.status != COBS_DECODE_OK);                               
    }
    {
        uint8_t inmsg[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
        uint8_t outmsg[16];
        memset(outmsg, 0xff, 16);
        uint8_t outmsg2[16];
        memset(outmsg2, 0xff, 16);
        cobs_encode_result re = cobs_encode(outmsg, 16, inmsg, 8);
        assert(re.status == COBS_ENCODE_OK);
        assert(re.out_len == 9);
        // Make sure we did't write past the end
        assert(outmsg[9] == 0xff);
        cobs_decode_result rd = cobs_decode(outmsg2, 16, outmsg, re.out_len);
        assert(rd.status == COBS_DECODE_OK);                               
        assert(rd.out_len == 8);                               
        // Make sure we did't write past the end
        assert(outmsg2[8] == 0xff);
        //cout << "Decoded Len " << rc2 << endl;    
        //prettyHexDump(outmsg2, rc2, cout);
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
    {
        uint8_t inmsg[8] = { 0, 2, 3, 0xff, 5, 6, 7, 0 };
        uint8_t outmsg[16];
        uint8_t outmsg2[16];
        cobs_encode_result re = cobs_encode(outmsg, 16, inmsg, 8);
        assert(re.status == COBS_ENCODE_OK);
        cobs_decode_result rd = cobs_decode(outmsg2, 16, outmsg, re.out_len);
        assert(rd.status == COBS_DECODE_OK);                               
        assert(rd.out_len == 8);                               
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
    {
        uint8_t inmsg[8] = { 0, 2, 3, 0xff, 5, 6, 7, 0xff };
        uint8_t outmsg[16];
        uint8_t outmsg2[16];
        cobs_encode_result re = cobs_encode(outmsg, 16, inmsg, 8);
        assert(re.status == COBS_ENCODE_OK);
        cobs_decode_result rd = cobs_decode(outmsg2, 16, outmsg, re.out_len);
        assert(rd.status == COBS_DECODE_OK);                               
        assert(rd.out_len == 8);                               
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
    {
        uint8_t inmsg[8] = { 1, 2, 3, 0xff, 5, 6, 7, 1 };
        uint8_t outmsg[16];
        uint8_t outmsg2[16];
        cobs_encode_result re = cobs_encode(outmsg, 16, inmsg, 8);
        assert(re.status == COBS_ENCODE_OK);
        cobs_decode_result rd = cobs_decode(outmsg2, 16, outmsg, re.out_len);
        assert(rd.status == COBS_DECODE_OK);                               
        assert(rd.out_len == 8);                               
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
    {
        uint8_t inmsg[8] = { 0,0,0,0,0,0,0,0 };
        uint8_t outmsg[16];
        uint8_t outmsg2[16];
        cobs_encode_result re = cobs_encode(outmsg, 16, inmsg, 8);
        assert(re.status == COBS_ENCODE_OK);
        cobs_decode_result rd = cobs_decode(outmsg2, 16, outmsg, re.out_len);
        assert(rd.status == COBS_DECODE_OK);                               
        assert(rd.out_len == 8);                               
        assert(memcmp(inmsg, outmsg2, 8) == 0);
    }
}
