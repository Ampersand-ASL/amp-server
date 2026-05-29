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
#pragma once

#include <functional>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace kc1fsz {

class Log;
class LineIAX2;
class LineUsb;
class LineSDRC;
class LocalRegistryStd;
class LocalAuthenticatorStd;

namespace amp {

class SignalIn;
class Bridge;
class WebUi;
class SignalOut;
class SignalIn;

/**
 * Utility function for pulling a string parameter out of the config JSON
 */
bool getCfgString(json cfg, const char* name, std::function<void(const char* c)> f);

/**
 * Utility function for pulling an integer parameter out of the config JSON
 */
bool getCfgUint(json cfg, const char* name, std::function<void(unsigned u)> f);

/**
 * Transfers the configuration settings in a JSON document to all of the 
 * various components in the system.
 *
 * @throws json::exception On a JSON error (i.e. missing element)
 */
int configHandler(Log& log, const json& cfg, WebUi& webUi, LineIAX2& iax2Channel1, 
    LocalRegistryStd& locReg, LocalAuthenticatorStd& locAuth,
    LineUsb& radio2, SignalIn& signalIn, SignalOut& signalOut, Bridge& bridge10, LineSDRC&,
    int iaxPortOverride);
}

}
