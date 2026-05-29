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
#include "serial-map.h"

// amp-core
#include "WebUi.h"
#include "LineIAX2.h"
#include "LineUsb.h"
#include "LineSDRC.h"
#include "SignalIn.h"
#include "SignalOut.h"
#include "Bridge.h"

// amp-server
#include "LocalAuthenticatorStd.h"
#include "config-handler.h"

using namespace std;

namespace kc1fsz {
    namespace amp {

bool getCfgString(json cfg, const char* name, std::function<void(const char* c)> f) {
    if (cfg.contains(name) && cfg[name].is_string()) {
        f(cfg[name].get<std::string>().c_str());
        return true;
    } else {
        return false;
    }
}

bool getCfgUint(json cfg, const char* name, std::function<void(unsigned u)> f) {
    if (cfg.contains(name) && cfg[name].is_string()) {
        try {
            int u = std::stoi(cfg[name].get<std::string>());
            f(u);
            return true;
        } catch (std::invalid_argument&) {
            return false;
        }
    } else {
        return false;
    }
}

/**
 * This function is solely responsible for taking the configuration document (JSON)
 * and applying it to the entire system. This will be called at startup and at
 * any time that the configuration is changed.
 */
int configHandler(Log& log, const json& cfg, WebUi& webUi, LineIAX2& iax2Channel1, 
    LocalRegistryStd& locReg, LocalAuthenticatorStd& locAuth,
    LineUsb& radio2, SignalIn& signalIn, SignalOut& signalOut, Bridge& bridge10, 
    LineSDRC& sdrcLine5, int iaxPortOverride) {

    // Transfer the new configuration into the various places it is needed
    webUi.setConfig(cfg);

    //iax2Channel1.setDNSRoot(getenv("AMP_ASL_DNS_ROOT"));

    // Pull out each part of the configuration document and route it to the correct
    // application object.

    getCfgString(cfg, "callsign", [&radio2](const char* c) { radio2.setCallsign(c); });
    getCfgUint(cfg, "hangDelay",  [&radio2](unsigned i) { radio2.setHangDelay(i); });
    getCfgString(cfg, "courtesyTone", [&radio2](const char* c) { radio2.setCourtesyTone(c); });
    getCfgUint(cfg, "courtesyDelay",  [&radio2](unsigned i) { radio2.setCourtesyDelay(i); });
    getCfgString(cfg, "node", [&log, &bridge10, &iax2Channel1](const char* localNode) {
        if (localNode[0] != 0) {
            log.important("Local node is %s", localNode);
            bridge10.setLocalNodeNumber(localNode);
            // #### TODO: MULTIPLE NODES AS SOME POINT
            iax2Channel1.setPokeNodeNumber(localNode);
        }
    });

    // Kerchunk filter configuration
    if (cfg.contains("kfnodes")) {
        // The nodes are comma-separated
        string kfnodes = cfg["kfnodes"].get<std::string>();
        vector<string> l;
        // This is a comma-delimited list
        std::istringstream tokenStream(kfnodes);
        string token;
        while (std::getline(tokenStream, token, ',')) {
            trim(token);
            if (token.empty())
                continue;
            l.push_back(token);
        }
        bridge10.setKerchunkFilterNodes(l);
    }

    if (cfg.contains("kfdelay")) {
        // The nodes are comma-separated
        string kfdelay = cfg["kfdelay"].get<std::string>();
        bridge10.setKerchunkFilterDelayMs(stoi(kfdelay));
    }

    if (cfg.contains("callsign"))
        iax2Channel1.setCallSign(cfg["callsign"].get<std::string>().c_str());
    if (cfg.contains("privateKey"))
        iax2Channel1.setPrivateKey(cfg["privateKey"].get<std::string>().c_str());
    if (cfg.contains("authFile"))
        locAuth.load(cfg["authFile"].get<std::string>().c_str());

    int iaxPort = iaxPortOverride;
    if (iaxPort == 0) {
        if (!cfg["iaxPort"].is_string())
            throw invalid_argument("iaxPort is missing/invalid");
        iaxPort = std::stoi(cfg["iaxPort"].get<std::string>());
    }
    
    int rc = iax2Channel1.open(AF_INET, iaxPort);
    if (rc < 0) 
        log.error("Failed to open IAX2 line %d", rc);
    else
        log.important("Opened IAX connection on port %d", iaxPort);

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
        if (aslAudioDevice.starts_with("usbaud ")) {
            int alsaCard;
            string ossDevice;
            // The leading "usbaud " is not part of the query that this function can handle
            int rc2 = resolveUSBSoundDevice(aslAudioDevice.substr(7).c_str(), alsaCard, ossDevice);
            if (rc2 < 0) {
                log.error("Unable to resolve audio device [%s] %d", aslAudioDevice.c_str(), rc2);
            } 
            else {
                log.important("Audio device [%s] mapped to ALSA card %d", aslAudioDevice.c_str(), alsaCard);                         

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

                if (!cfg["duplexmode"].is_string())
                    throw invalid_argument("duplexmode is missing/invalid");
                int duplexMode = std::stoi(cfg["duplexmode"].get<std::string>());

                if (!cfg["echogain"].is_string())
                    throw invalid_argument("echogain is missing/invalid");
                float echoGainDb = std::stof(cfg["echogain"].get<std::string>());

                // #### TODO: MAKE ECHO CONFIGURABLE
                rc = radio2.open(alsaCard, txMixASet, txMixBSet, rxMixerSet, duplexMode == 1,
                    echoGainDb);
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
        string aslCosDevice;
        if (cfg["aslCosDevice"].is_string()) 
            aslCosDevice = cfg["aslCosDevice"].get<std::string>();
        string aslCosSignal;
        if (cfg["aslCosSignal"].is_string()) 
            aslCosSignal = cfg["aslCosSignal"].get<std::string>();
        
        if (!aslCosDevice.empty()) {
            if (aslCosDevice.starts_with("usbaud ")) {
                string cosDev;
                int rc3 = resolveUSBHIDDevice(log, aslCosDevice.substr(7).c_str(), cosDev);
                if (rc3 < 0) {
                    log.error("Unable to resolve COS HID [%s] %d", aslCosDevice.c_str(), rc3);
                    return -1;
                } 
                else {
                    log.important("COS HID [%s] mapped to [%s]", aslCosDevice.c_str(), cosDev.c_str());
                    rc = signalIn.openHid(cosDev.c_str(), aslCosSignal.c_str());
                    if (rc < 0) {
                        log.error("Failed to open HID signal in connection %d", rc);
                        return -1;
                    }
                }
            }
            else if (aslCosDevice.starts_with("usbser ")) {
                string cosDev;
                int rc3 = resolveUSBSerialDevice(aslCosDevice.substr(7).c_str(), cosDev);
                if (rc3 < 0) {
                    log.error("Unable to resolve COS Serial [%s] %d", aslCosDevice.c_str(), rc3);
                    return -1;
                } 
                else {
                    log.important("COS Serial [%s] mapped to [%s]", aslCosDevice.c_str(), cosDev.c_str());
                    rc = signalIn.openSerial(cosDev.c_str(), aslCosSignal.c_str());
                    if (rc < 0) {
                        log.error("Failed to open serial signal in connection %d", rc);
                        return -1;
                    }
                }
            }

            // Deal with signal inversion
            string cosInvert;
            if (cfg["aslCosInvert"].is_string()) 
                cosInvert = cfg["aslCosInvert"].get<std::string>();
            signalIn.setInvert(cosInvert == "1");
        }
        else {
            signalIn.close();
        }

        // Resolve the PTT signal
        string aslPttDevice;
        if (cfg["aslPttDevice"].is_string()) 
            aslPttDevice = cfg["aslPttDevice"].get<std::string>();
        string aslPttSignal;
        if (cfg["aslPttSignal"].is_string()) 
            aslPttSignal = cfg["aslPttSignal"].get<std::string>();

        if (!aslPttDevice.empty()) {

            // Deal with signal inversion. This is done first because the open() will 
            // assert the initial state of the signal.
            string pttInvert;
            if (cfg["aslPttInvert"].is_string()) 
                pttInvert = cfg["aslPttInvert"].get<std::string>();
            signalOut.setInvert(pttInvert == "1");

            if (aslPttDevice.starts_with("usbaud ")) {
                string pttDev;
                int rc3 = resolveUSBHIDDevice(log, aslPttDevice.substr(7).c_str(), pttDev);
                if (rc3 < 0) {
                    log.error("Unable to resolve PTT HID [%s] %d", aslPttDevice.c_str(), rc3);
                    return -1;
                } 
                else {
                    log.important("PTT HID [%s] mapped to [%s]", aslPttDevice.c_str(),
                        pttDev.c_str());
                    rc = signalOut.openHid(pttDev.c_str(), aslPttSignal.c_str());
                    if (rc < 0) {
                        log.error("Failed to open HID signal out connection %d", rc);
                        return -1;
                    }
                }
            }
            else if (aslPttDevice.starts_with("usbser ")) {
                string pttDev;
                int rc3 = resolveUSBSerialDevice(aslPttDevice.substr(7).c_str(), pttDev);
                if (rc3 < 0) {
                    log.error("Unable to resolve PTT Serial [%s] %d", aslPttDevice.c_str(), rc3);
                    return -1;
                } 
                else {
                    log.important("PTT Serial [%s] mapped to [%s]", aslPttDevice.c_str(), pttDev.c_str());
                    rc = signalOut.openSerial(pttDev.c_str(), aslPttSignal.c_str());
                    if (rc < 0) {
                        log.error("Failed to open serial signal in connection %d", rc);
                        return -1;
                    }
                }
            }
        }
        else {
            signalOut.close();
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