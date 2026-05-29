#include <iostream>

#include "config-handler.h"

using namespace std;
using namespace kc1fsz;

static void testCfg() {

    json cfg;
    cfg["a"] = "izzy";   
    cfg["b"] = "1000";

    bool hit = false;
    amp::getCfgString(cfg, "a", [&hit](const char* c) { assert(strcmp(c, "izzy") == 0); hit = true; });
    assert(hit);

    hit = false;
    amp::getCfgString(cfg, "c", [&hit](const char* c) { hit = true; });
    assert(!hit);

    hit = false;
    amp::getCfgUint(cfg, "b", [&hit](unsigned u) { assert(u == 1000); hit = true; });
    assert(hit);

    hit = false;
    amp::getCfgUint(cfg, "a", [&hit](unsigned u) { hit = true; });
    assert(!hit);
}

int main(int, const char**) {
    testCfg();
}

