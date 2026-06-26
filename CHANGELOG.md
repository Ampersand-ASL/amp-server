# Change Log

## 20260626

* Silence compiler warnings on DecTalk library.

## 20260610

* Changed internal timing to use a monotonic clock instead of a real-time clock.
This was done to avoid some strange behavior observed when the NTP-based system
clock adjustments were happening behind the scenes, particularly during initial 
boot.

## 20260608

* Enabled Log tab on web UI.

## 20260608

* Added !!DISCONNECT!! support for HamVOIP compatibility.
* Removed T TALKERID text message.
* Cleared up a defect found by Steve KD3CSK. Startup was crashing
if no USB sound device was attached.

## 20260607

* No longer re-transmitting NEW messages on initial connection.
* Cleaned up a few more IAX protocol issues.

## 20260601

* Addressed some problems with communications on non-USB serial ports 
in order to support SHARI programming cleanly.

## 20260527

* Added authentication file for private nodes.
* Enabled MD5 authentication challenges or private nodes.
* Improved support for explicit URI connection strings.
* Enabled Ed25519 authentication based on ampr.org public keys. This takes
priority over source-IP validation if available.
* Improved favorites system to include support for explicit URI connection strings. This 
is particularly relevant for private node connections.
* Addressed disconnect/reconnect problems pointed out by Patrick N2DYI.
* Added a "Running" indication on the home page per request of David NR9V.
* Now writing basic startup information and error messages into the syslog.
* Made some changes to the home page HTML to improve accessibility for screen readers.

## 20260520

* Fixed 61057 POKE port number.
* Addressed a protocol violation with PING/PONG. PONG timestamp is now 
matching PING.

## 20260516

* Lots of refactoring of USB audio to try to resolve overrun problem
demonstrated by David NR9V on his Arduino Uno Q.
* Improvements to DTMF handling.
* Corrected some bugs that caused calls to be dropped during unrelated
configuration changes.

## 20260512

* Improved recovery from USB plug/unplug situation.
* Added debounce on PTT signal to improve Shari/ANH performance. 
* Cleaned up a logic error with re-transmit, now holding all messages (except ACK)
in the retransmit buffer, regardless of whether ACK is needed.
* Per request of Joe KA9OPL, added audio feedback on DTMF buttons.

## 20260511

* Improved the management of the USB audio buffer on playback.

## 20260510

* Fixed a problem with USB device hierarchy pointed out by Patrick N2DYI. Tested
using AllScan UCI80, UCI90, URI110, and ANH95.

## 20260505

* Addressed a problem with sequence-number wrap-around that would cause disconnects.
* Major change to the way COS/PTT signals are configured. Now fully supporting serial
control for these signals.

## 2026-04-28

* Refactoring to simplify retransmission buffer. Found an issue with handling
of VNAK and fixed it.

## 2026-04-27

* Release for Patrick N2DYI, working on resolving a delay issue that shows
up on the local echo (sidetone) feature after the first transmission.

## 2026-03-05

* Added local echo duplex mode and local echo gain.

## 2026-02-25

* Problems with the 8K linear CODEC have been addressed.
* The accessibility of the user interface has been improved. (Thanks
to Joe KA9OPL for his help on this)
* By popular demand, initial support for SA818/SHARI-style devices has been added.
* Node statistics have been enabled. 

## 2026-02-16

* The --httppwd command-line option was added to allow a password to be set
for the user interface. By default there is no authentication required. Thanks to 
Smitty WB1G for this suggestion.
* Internal changes have been made to improve performance and scalability.
* Problems with CODEC negotiation have been addressed.
* Support for the G.726 AAL2 CODEC has been added.

## 2026-02-07

* Major update to the user interface.  Important points:
  - There are now status/error messages on the bottom of the screen to
    provide feedback when connections fail.
  - The list of connected nodes has been enhanced to include active
    talker (more later) and the list of linked nodes as reported by
    the "L" text message.
  - Any nodes that that are actively talking will be highlighted in blue.
  - Favorites can now be specified on the configuration page in
    comma-separated "nodenumber:text description" format. For example
    "61057:ASL Parrot".
  - A callsign can now be entered on the configuration page. This is
    used as the Talker ID for transmissions originating from the
    hardware connected to your node.
  - The kerchunk filter can be selectively enabled for the comma-separated
    list of nodes entered on the configuration page.
* Added outbound L telemetry text message to allow other nodes to see the linked stations
* An initial implementation of the Talker ID feature has been provided. This
will pass the callsign of the speaker through the network. Obviously,
this only works for Ampersand nodes.
* The coefficients on the various low-pass filters have been refined to improve
audio performance.

## 2026-01-30

- Changed shape of low-pass filters used for decimation/interpolation
to improve audio quality.

## 2026-01-28

- Web UI switched to dark mode per NR9V suggestion.
- Added audio level meters to Web UI.
- Enabled DTMF keypad on Web UI.
- Added high-speed serial audio interface for SDRC integration.
- Addresses a few defects related to connection management.
