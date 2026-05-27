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
#pragma once

#include <string>
#include <vector>
#include <utility>

#include "kc1fsz-tools/fixedstring.h"

#include "LineIAX2.h"

namespace kc1fsz {

class Log;

class LocalAuthenticatorStd : public LocalAuthenticator {
public:

    LocalAuthenticatorStd(Log& log);

    bool load(const char* filename);

    fixedstring getSecret(const char* targetNode, const char* username) const;

private:

    Log& _log;
    std::vector<std::pair<std::string,std::string>> _store;
};

}
