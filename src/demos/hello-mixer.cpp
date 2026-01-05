#include <iostream>
#include "LineUsb.h"

using namespace std;
using namespace kc1fsz;

int main(int, const char**) {

    int max = getMixerMax("hw:2", "Speaker Playback Volume"); 
    cout << "Max " << max << endl;
    int rc = setMixer("hw:2", "Speaker Playback Volume", 500 * max / 1000 , 500 * max / 1000);
    cout << rc << endl;
}
