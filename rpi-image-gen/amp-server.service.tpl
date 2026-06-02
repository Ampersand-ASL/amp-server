# Install in: /lib/systemd/system/amp-server.service:
[Unit]
Description=Ampersand Server
After=network.target
StartLimitIntervalSec=0

[Service]
Type=simple
Restart=always
RestartSec=1
User=amp
ExecStart=/usr/bin/amp-server
WorkingDirectory=/usr/bin
RestrictRealtime=off
# Make the process real-time with high priority
# The + requests higher privileges.
ExecStartPost=+/bin/sh -c "/usr/bin/chrt --rr -p 50 $MAINPID"

[Install]
WantedBy=multi-user.target
