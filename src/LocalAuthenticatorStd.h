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

#include <cstring>

#include "kc1fsz-tools/fixedstring.h"

#include "LineIAX2.h"

namespace kc1fsz {

// #### TODO: Need a real implementation for this.
class LocalAuthenticatorStd : public LocalAuthenticator {
public:
    virtual fixedstring getSecret(const char* targetNode, const char* username) const {
        return fixedstring("999");
    }
};

}
