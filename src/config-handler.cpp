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
#include <stdexcept>

#include "sound-map.h"

// amp-core
#include "WebUi.h"
#include "LineIAX2.h"
#include "LineUsb.h"
#include "LineSDRC.h"
#include "SignalIn.h"
#include "Bridge.h"

// amp-server
#include "config-handler.h"

using namespace std;

namespace kc1fsz {

    namespace amp {

int configHandler(Log& log, const json& cfg, WebUi& webUi, LineIAX2& iax2Channel1, 
    LocalRegistryStd& locReg,
    LineUsb& radio2, SignalIn& signalIn3, Bridge& bridge10, LineSDRC& sdrcLine5,
    int iaxPortOverride) {

    // Transfer the new configuration into the various places it is needed
    webUi.setConfig(cfg);

    //iax2Channel1.setPrivateKey(getenv("AMP_PRIVATE_KEY"));
    //iax2Channel1.setDNSRoot(getenv("AMP_ASL_DNS_ROOT"));
    
    if (cfg.contains("node")) {
        string localNode = cfg["node"];
        if (!localNode.empty())
            bridge10.setLocalNodeNumber(localNode.c_str());
    }

    int iaxPort = iaxPortOverride;
    if (iaxPort == 0) {
        if (!cfg["iaxPort"].is_string())
            throw invalid_argument("iaxPort is missing/invalid");
        iaxPort = std::stoi(cfg["iaxPort"].get<std::string>());
    }
    
    int rc = iax2Channel1.open(AF_INET, iaxPort, "radio");
    if (rc < 0) {
        log.error("Failed to open IAX2 line %d", rc);
    }

    /*
    //if (!cfg["sdrcSerialDevice"].is_string()) {
        rc = sdrcLine5.open("/dev/ttyUSB0");
        if (rc < 0) {
            log.error("Failed to open SDRC line %d", rc);
        }
    //}
    */
    
    string setupMode = cfg["setupMode"].get<std::string>();

    // ----- ASL Compatibility Mode -----------------------------------

    if (setupMode.empty() || setupMode == "0") {

        // Resolve the audio device
        string aslAudioDevice = cfg["aslAudioDevice"].get<std::string>();
        if (aslAudioDevice.starts_with("usb ")) {
            int alsaCard;
            string ossDevice;
            int rc2 = querySoundMap(aslAudioDevice.substr(4).c_str(), alsaCard, ossDevice);
            if (rc2 < 0) {
                log.error("Unable to resolve sound device %d", rc2);
            } 
            else {
                log.info("Audio %s mapped to ALSA card %d", 
                    aslAudioDevice.c_str(), alsaCard);                         

                // NOTE: ASL uses 0-1000 scale
                if (!cfg["aslTxMixASet"].is_string())
                    throw invalid_argument("aslTxMixASet is missing/invalid");
                int txMixASet = std::stoi(cfg["aslTxMixASet"].get<std::string>());

                if (!cfg["aslTxMixBSet"].is_string())
                    throw invalid_argument("aslTxMixBSet is missing/invalid");
                int txMixBSet = std::stoi(cfg["aslTxMixBSet"].get<std::string>());
                
                if (!cfg["aslRxMixerSet"].is_string())
                    throw invalid_argument("aslRxMixerSet is missing/invalid");
                int rxMixerSet = std::stoi(cfg["aslRxMixerSet"].get<std::string>());

                rc = radio2.open(alsaCard, txMixASet, txMixBSet, rxMixerSet);
                if (rc < 0) {
                    if (rc == -12)
                        log.error("Unable to open sound device, busy");
                    else 
                        log.error("Unable to open sound device");
                    return -1;
                }
            }
        }

        // Resolve the COS signal
        string aslCosFrom = cfg["aslCosFrom"].get<std::string>();
        if (aslAudioDevice.starts_with("usb ") && aslCosFrom.starts_with("usb")) {

            string cosSignalDevice;
            int rc3 = queryHidMap(aslAudioDevice.substr(4).c_str(), cosSignalDevice);
            if (rc3 < 0) {
                log.error("Unable to resolve HID device %d", rc3);
                return -1;
            } 
            else {
                log.info("HID %s mapped to %s", aslAudioDevice.c_str(),
                    cosSignalDevice.c_str());
                rc = signalIn3.openHid(cosSignalDevice.c_str());
                if (rc < 0) {
                    log.error("Failed to open HID signal connection %d", rc);
                    return -1;
                }
            }

            // ##### TODO: DEAL WITH INVERT
        }
    }
    else {
        log.error("Setup mode invalid: %s", setupMode.c_str());
        return -1;
    }

    return 0;
}

    }
}