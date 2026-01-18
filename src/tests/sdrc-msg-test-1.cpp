#include <stddef.h>
#include <stdint.h>
#include <assert.h>

#include <iostream>
#include <cstring>

#include "kc1fsz-tools/Common.h"
#include "DigitalAudioPortRxHandler.h"
#include "cobs.h"

using namespace std;
using namespace kc1fsz;

int main(int, const char**) {
    {
        uint8_t payload[PAYLOAD_SIZE];
        for (unsigned i = 0; i < PAYLOAD_SIZE; i++)
            payload[i] = 0x18;
        uint8_t msg[NETWORK_MESSAGE_SIZE];
        DigitalAudioPortRxHandler::encodeMsg(payload, PAYLOAD_SIZE, 
            msg, NETWORK_MESSAGE_SIZE);
        // Message header and type 
        assert(msg[0] == 0 && msg[1] == 0x01);
        // This should be the long COBS case
        assert(msg[2] == 2);
        // Make sure there are no zeros
        for (unsigned i = 1; i < NETWORK_MESSAGE_SIZE; i++)
            assert(msg[i] != 0);
        // Reverse
        uint8_t payload2[PAYLOAD_SIZE];
        assert(DigitalAudioPortRxHandler::decodeMsg(msg, NETWORK_MESSAGE_SIZE,
            payload2, PAYLOAD_SIZE) == 0);
        assert(memcmp(payload, payload2, PAYLOAD_SIZE) == 0);
    }
    {
        uint8_t payload[PAYLOAD_SIZE];
        for (unsigned i = 0; i < PAYLOAD_SIZE; i++)
            payload[i] = (i % 2 == 0) ? 0 : 0x18;
        uint8_t msg[NETWORK_MESSAGE_SIZE];
        DigitalAudioPortRxHandler::encodeMsg(payload, PAYLOAD_SIZE, 
            msg, NETWORK_MESSAGE_SIZE);
        // Message header and type 
        assert(msg[0] == 0 && msg[1] == 0x01);
        // This should be the short COBS case
        assert(msg[2] == 1);
        // Make sure there are no zeros
        for (unsigned i = 1; i < NETWORK_MESSAGE_SIZE; i++)
            assert(msg[i] != 0);
        // Reverse
        uint8_t payload2[PAYLOAD_SIZE];
        assert(DigitalAudioPortRxHandler::decodeMsg(msg, NETWORK_MESSAGE_SIZE,
            payload2, PAYLOAD_SIZE) == 0);
        assert(memcmp(payload, payload2, PAYLOAD_SIZE) == 0);
    }
}
