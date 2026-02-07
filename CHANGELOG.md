# Change Log

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
