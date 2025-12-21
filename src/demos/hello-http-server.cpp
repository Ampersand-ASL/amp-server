//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

#include <unistd.h>
#include <iostream>

using namespace std;

void srv_thread(void*) {

    // HTTP
    httplib::Server svr;

    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
    res.set_content("Hello Izzy!", "text/plain");
    });

    svr.listen("0.0.0.0", 8080);
}

int main(int, const char**) {

    // Get the AMP thread running
    _beginthread(srv_thread, 0, (void*)0);

    while (true) {
        cout << "test" << endl;
        sleep(5);
    }

}

