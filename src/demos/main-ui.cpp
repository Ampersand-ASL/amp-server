#include <unistd.h>        
#include <thread>

#include "ui-thread.h"

int main(int, const char**) {

    std::thread t(ui_thread);
    while (true) {
        sleep(5);
    }
}

