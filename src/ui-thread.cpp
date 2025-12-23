/**
 * Copyright (C) 2025, Bruce MacKinnon KC1FSZ
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
//#include <unistd.h>
#include <iostream>
#include <thread>

#include <nlohmann/json.hpp>
#include "httplib.h"

using namespace std;

// https://github.com/yhirose/cpp-httplib
// https://github.com/nlohmann/json

using json = nlohmann::json;

void ui_thread() {

    //pthread_setname_np(pthread_self(), "amp-server-ui");

    // HTTP
    httplib::Server svr;

    // ------ Main Page --------------------------------------------------------

    bool ptt = false;

    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
        res.set_file_content("../www/index.html");
    });
    svr.Get("/status", [&ptt](const httplib::Request &, httplib::Response &res) {
        json o;
        o["cos"] = ptt;
        o["ptt"] = ptt;
        auto a = json::array();
        json o2;
        o2["node"] = "2222";
        auto b = json::array();
        b.push_back("61057");
        b.push_back("55553");
        o2["connections"] = b;
        a.push_back(o2);
        o["connections"] = a;
        res.set_content(o.dump(), "application/json");
    });
    svr.Post("/status-save", [&ptt](const httplib::Request &, httplib::Response &res, 
        const httplib::ContentReader &content_reader) {
        cout << "Save Status" << endl;
        ptt = !ptt;
    });
    // ------ Config Page-------------------------------------------------------

    svr.Get("/config", [](const httplib::Request &, httplib::Response &res) {
        res.set_file_content("../www/config.html");
    });
    svr.Get("/config-load", [](const httplib::Request &, httplib::Response &res) {
        json o;
        o["node"] = "61057";
        o["password"] = "xxxxxx";
        o["audiodevice"] = "bus:1,port:3";
        o["iaxport4"] = 4569;
         res.set_content(o.dump(), "application/json");
    });
    svr.Post("/config-save", [](const httplib::Request &, httplib::Response &res, 
        const httplib::ContentReader &content_reader) {
        cout << "Saving changes" << endl;
        std::string body;
        content_reader([&](const char *data, size_t data_length) {
            body.append(data, data_length);
            return true;
        });
        cout << body << endl;
    });
    svr.Get("/audiodevice-list", [](const httplib::Request &, httplib::Response &res) {
        auto a = json::array();
        json o;
        o["bus"] = "1";
        o["port"] = "2";
        o["query"] = "bus:1,port:2";
        o["desc"] = "C-Media Electronics, Inc. USB Audio Device";
        a.push_back(o);
        o["bus"] = "1";
        o["port"] = "3";
        o["query"] = "bus:1,port:3";
        o["desc"] = "Other";
        a.push_back(o);
         res.set_content(a.dump(), "application/json");
    });

    svr.listen("0.0.0.0", 8080);
}
