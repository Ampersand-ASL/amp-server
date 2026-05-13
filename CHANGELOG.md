# Change Log

## 20250512

* Improved recovery from USB plug/unplug situation.
* Added debounce on PTT signal to improve Shari/ANH performance. 
* 

## 20250511

* Improved the management of the USB audio buffer on playback.

## 20250510

* Fixed a problem with USB device hierarchy pointed out by Patrick N2DYI. Tested
using AllScan UCI80, UCI90, URI110, and ANH95.

## 20250505

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
