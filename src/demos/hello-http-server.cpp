#include <unistd.h>
#include <iostream>
#include <thread>

//#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib.h"

using namespace std;

void srv_thread() {

    cout << "Starting ..." << endl;

    // HTTP
    httplib::Server svr;

    //svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
    //    res.set_content("Hello Izzy 2!", "text/plain");
    //});

    auto ret = svr.set_mount_point("/", "../src/demos/www");
    if (!ret) {
        cout << "Error" << endl;
        return;
    }

    svr.listen("0.0.0.0", 8080);
}

int main(int, const char**) {

    // 1. Create and start the thread
    std::thread t(srv_thread);

    while (true) {
        sleep(5);
    }
}

