/**
 * Copyright (C) 2026, Bruce MacKinnon KC1FSZ
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
#include <fstream>
#include <sstream>
#include <vector>

#include "kc1fsz-tools/Common.h"
#include "kc1fsz-tools/Log.h"

#include "LocalAuthenticatorStd.h"

using namespace std;

namespace kc1fsz {

LocalAuthenticatorStd::LocalAuthenticatorStd(Log& log)
:   _log(log) {
}

bool LocalAuthenticatorStd::load(const char* filename) {

    _log.info("Loading authentication file %s", filename);

    ifstream inf(filename);
    if (!inf.is_open()) {
        _log.error("Authentication file not loaded");
        return false;
    }

    _store.clear();

    string line;
    while (getline(inf, line)) {
        trim(line);
        // Ignore comments
        if (line.empty() || line.starts_with("#"))
            continue;
        stringstream ss(line);
        string word;
        vector<string> tokens;
        while (ss >> word)
            tokens.push_back(word);
        if (tokens.size() >= 2)
            _store.push_back(make_pair(tokens.at(0), tokens.at(1)));
    }
    return true;
}

fixedstring LocalAuthenticatorStd::getSecret(const char* targetNode, const char* username) const {
    for (auto a : _store)
        if (a.first == username)
            return fixedstring(a.second.c_str());
    return fixedstring();
}

}
