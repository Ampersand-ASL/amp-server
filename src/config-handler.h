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

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace kc1fsz {

class Log;
class LineIAX2;
class LineUsb;
class LineSDRC;

namespace amp {

class SignalIn;
class Bridge;

/**
 * Transfers the configuration settings in a JSON document to all of the 
 * various components in the system.
 *
 * @throws json::exception On a JSON error (i.e. missing element)
 */
int configHandler(Log& log, const json& cfg, WebUi& webUi, LineIAX2& iax2Channel1, 
    LineUsb& radio2, SignalIn& signalIn3, Bridge& bridge10, LineSDRC&);

}

}
