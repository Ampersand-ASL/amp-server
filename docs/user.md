# Ampersand Server User/Install Documentation

At the moment the Ampersand Server (amp-server) provides a basic [All Star Link](https://www.allstarlink.org/) node for desktop radio-less and simple hot-spot use. Future releases will enable more functionality. Send
comments/questions to Bruce MacKinnon (KC1FSZ) using the e-mail address in [QRZ](https://www.qrz.com/db/KC1FSZ).

This is experimental work that explores the potential of ASL linking 
without the use of the Asterisk PBX system. [Project documentation is here](https://mackinnon.info/ampersand/). 

All of the testing of this system is happening on either:
* A Raspberry Pi 5 running Debian 12 Bookworm. This is an ARM-64 platform.
* A Dell Wyse 3040 mini-PC running Debian 13 Trixie. This is an x86-64 platform.

A separate build for ARM Cortex-M33 microcontroller boards has also been tested.
This will be documented separately.

All of my Linux testing has been done using an [AllScan](https://allscan.info/) UCI90 audio interface or the [Repeater Builder](https://www.repeater-builder.com/products/stm32-dvm.html) RB-USB RIM Lite module. Both are based on the C-Media C1xx audio 
chip. 

If you are looking for a cloud-based "virtual or "hub" server, the [Ampersand Hub Project](https://github.com/Ampersand-ASL/amp-hub) is a 
better choice for you.

The [change log is located here](../CHANGELOG.md). I try to keep it up to date.

> [!IMPORTANT]
> If you are using the AllStarLink system please [make a dontation](https://www.allstarlink.org/about/donate.php) to support the network. 

Network Setup (IPv4)
====================

There are other detailed sources of information about ASL network configuration so 
I won't repeat everything here. Bottom line:

* Make sure you are clear on what IAX (UDP) port your node is using. This assignment
happens on the [ASL Portal](https://www.allstarlink.org/portal/servers.php). UDP port 4569 is the common default.
* Make sure that your IAX port is properly configured on the Ampersand Configuration 
tab (see below).
* If you expect to receive inbound calls make sure that your IAX port is opened/forwarded through your firewall/NAT system.
* If you expect to receive inbound calls make sure that your IAX port is opened on any Linux/Windows firewall tools that are 
running on your machine (if applicable).
* You can test your network connection using the 61057 parrot. If the 61057 parrot
tells you that your "network test succeeded" that means that your firewall is open
and that you can accept inbound calls.

Network Setup (IPv6)
====================

(To follow shortly)

Installation Instructions (Linux)
=================================

Install required packages:

    sudo apt install wget net-tools libcurl4-gnutls-dev

An adjustment needs to be made to allow non-root users to access the HID interfaces. This is relevant to the COS/PTT signals. Create /etc/udev/rules.d/99-mydevice.rules with this contents:

    # The C-Media vendor ID
    SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0d8c", MODE="0666", TAG+="uaccess"

(And include any other devices you plan to use. I've noticed that the ASL3 installer
also includes vendor ID 1209 in their configuration.)

Reboot, or just force reload of rules:

    sudo udevadm control --reload-rules
    sudo udevadm trigger

If you are already running the Ampersand Server (i.e. this is an upgrade),
you will need to shut down the service:

    sudo systemctl stop amp-server

Installation steps:

    export AMP_SERVER_VERSION=20260608
    export AMP_ARCH=$(uname -m)
    wget https://ampersand-asl.s3.us-west-1.amazonaws.com/releases/amp-${AMP_SERVER_VERSION}-${AMP_ARCH}.tar.gz
    tar xvf amp-${AMP_SERVER_VERSION}-${AMP_ARCH}.tar.gz
    ln -s amp-${AMP_SERVER_VERSION}-${AMP_ARCH} amp

In case you need the links:

* The latest package for arm-64 is here: [https://ampersand-asl.s3.us-west-1.amazonaws.com/releases/amp-20260608-aarch64.tar.gz](https://ampersand-asl.s3.us-west-1.amazonaws.com/releases/amp-20260608-aarch64.tar.gz)
* The latest package for x86-64 is here: [https://ampersand-asl.s3.us-west-1.amazonaws.com/releases/amp-20260608-x86_64.tar.gz](https://ampersand-asl.s3.us-west-1.amazonaws.com/releases/amp-20260608-x86_64.tar.gz)

Running the Server (Linux)
==========================

    cd amp
    ./amp-server 

Command-line options should be used if you want to override defaults:

* --httport (defaults to 8080).  Used to change the port that the web UI runs on.
* --httppwd (defaults to none).  Used to set the password for access to the web UI. Username
is always "user."  Please pay attention to shell quoting rules when using passwords that 
contain special characters.
* --config (defaults $HOME/amp-server.json). Used to change the location of the configuration 
file. This is particularly useful when running more than one server on the same machine.
* --trace Used to turn on extended network tracing.
* --capture Used to turn on full network capture. Creates a .pcap file in the current 
working directory.

The server is operated via a web UI. Point your browser to the server using port 8080 (the default), or a different port if you
have configured one on the command line. If you've set a password you will be prompted to
log in (username is always "user")  The main screen will look like this:

![Amp1](amp-server-home.jpg)

Running as a Linux Service
==========================

You might want to run your AMP Server as a service. This is optional, but
it will keep the server running after you log out or reboot.

Copy the amp-server binary to /usr/bin.

Create a service file called /lib/systemd/system/amp-server.service that looks
something like this:

    [Unit]
    Description=AMP Server
    After=network.target
    StartLimitIntervalSec=0

    [Service]
    Type=simple
    Restart=always
    RestartSec=1
    # Change to your id, best not to run as root
    User=bruce
    # Can add an optional password here by including --httppwd "YOURPASSWORD"
    ExecStart=/usr/bin/amp-server
    WorkingDirectory=/tmp
    RestrictRealtime=off
    # Make the process real-time with high priority
    # The + requests higher privileges.
    ExecStartPost=+/bin/sh -c "/usr/bin/chrt --rr -p 50 $MAINPID"

    [Install]
    WantedBy=multi-user.target

Enable and start the service:

    sudo systemctl enable amp-server
    sudo systemctl start amp-server

You can view the log using this command:

    journalctl -u amp-server -f

Setup/Configuration (Linux)
===========================

Press "Configuration" at the top of the screen to get to the configuration tab
that looks like this:

![Amp2](amp-server-config.jpg)

This configuration should be very consistent with that used on the ASL system. Fill 
in your node number, password, and IAX port number. All other defaults should be enough to get you started.

### Audio/Signal Configuration

If you are planning to connect a radio/microphone/speaker to your Ampersand server you will need
to set a few things on the configuration tab:

* The "Audio Device" setting selects the hardware interface used for audio input/output. This will usually
be a CM10x device connected to a USB port. The setting will default to the first CM10x device found. If you 
have more than one USB audio device you should pay close attention to the USB port number (something like "1.4.2") 
to disambiguate devices.
* The "Carrier Detect Device" controls the hardware interface used to detect an incoming signal. This will be
either (a) your radio's carrier detect signal (b) your microphone PTT button on a radio-less station. The most
typical configuration uses the CM10x Volume Down signal. This mode is enabled by selecting your USB audio device
in the "Carrier Detect Device" menu and selecting "Volume Down" option in the "Carrier Detect Signal" menu. Some 
systems make use of the signaling interface of a serial port to capture the carrier detect signal. In this case
select your serial port in the "Carrier Detect Device" menu and select the appropriate signal (either CTS or DCD)
in the "Carrier Detect Signal" menu.
* The "PTT Device" controls the hardware interface used to key a radio. The most typical configuration uses the 
CM10x GPIO3 signal for this purpose. This mode is enabled by selecting your USB audio device
in the "PTT Device" menu and selecting the "GPIO3" option in the "PTT Signal" menu. Some 
systems make use of the signaling interface of a serial port to generate the PTT signal. In this case
select your serial port in the "PTT Device" menu and select the appropriate signal (either RTS or DTR)
in the "PTT Signal" menu.

### Audio Level Configuration

The audio levels will be the next thing to configure. Your audio level will be displayed in 
the system log any time you key your microphone (regardless of whether you are connected to 
any other nodes). The levels will be displayed like this:

![Amp3](amp-3.jpg)

The audio level that you are transmitting into the ASL network is controlled using the 
"Receive Mixer" level on the configuration tab. This is a bit confusing since
the "Receive" in this context is from the perspective of the radio interface hardware.

![Amp3](amp-4.jpg)

## Favorites Configuration

A user-defined list of frequently-called nodes can be configured. Green buttons will appear 
across the top of the home screen.

![Favorites 2](fav2.jpg)

This list is entered on the Configuration Tab. 

![Favorites 1](fav1.jpg)

The list should be comma-separated with a colon between the node number and text description.
For example:

    2002:ASL Parrot,61057:ASL Parrot,672731:AMP Hub,27339:East Cost Reflector,51018:W6EK SFARC

Be sure to press "Save" at the bottom of the screen after making a configuration change.

When an explicit connection URI is being used instead of a node number (explained below), the 
node number will contain colons and commas. To eliminate parsing ambiguity the URI should be 
enclosed in double-quotes. For example:

    2002:ASL Parrot,"iax:radio@192.168.8.143:4568/672732,none":Local

This will look like this on the home screen:

![Favorites 3](cfg2.jpg)

Audio Level Hints
=================

Setting audio levels can be tricky because of the many variables involved. There
are two things that need to be adjusted.

**The audio that you send to the AllStar network** which comes from either (a) your 
radio receiver or (b) your microphone on a radio-less node. This level is displayed
on the "USB RX" line of the level meter on the Home tab. It is important to adjust 
your node to avoid clipping of this audio. A good target is around -6dBFS peak.

**The audio that you receive from the AllStar network** which is sent to either (a)
your radio transmitter or (b) your speaker on a radio-less node. This level is 
displayed on the "USB TX" line of the level meter on the Home tab. It is important
to adjust your system to avoid excessive deviation on a transmitter. 

Use the 61057 parrot to test audio and get feedback on your level. Good receive audio
should peak around -6dB. 

> [!NOTE]
> The terminology can be counter-intuitive to users of radio-less nodes
since they typically think of the audio that comes from their microphone as "transmit" 
audio. Keep in mind that the terms are defined from the perspective of a radio-connected
system. When you key the microphone on a radio-less node the level being displayed
is what is being **received** from the microphone connection.

Network Debugging Hints
=======================

* Pay close attention to the UDP port number you are using. Each ASL Node 
number is associated with an ASL Server. Each ASL Server is assigned a 
UDP port for IAX traffic. Sometimes people get confused about this when they
start running multiple nodes.
* Just because your node can call out doesn't mean that you can accept 
calls. The firewall/NAT adjustments described above aren't required to 
make outgoing calls - only to receive incoming ones.
* A valid ASL registration is required for some nodes to accept your call.
*ASL parrots often do not require registration* so if you find that your
call is accepted by a parrot but not by other nodes it is likely that your 
registration is invalid. Check your password.
* The ASL registration process takes some time to propagate. When your node
first starts up your calls may not be accepted. Wait about 10 minutes and try again.
* Test using parrot 61057 **before asking for network help**. This parrot will provide 
information about whether (a) your node is registered and (b) whether your 
node is reachable from the outside.

# Setup of SA818-Based Hotspot (SHARI and Derivatives) 

I know these devices have a mixed reputation (depending on supplier) so I'm 
not necessarily advocating/recommending any particular unit. Ampersand has 
been tested with a few SHARI variants and it seems to work fine. 

My SA818 reports this version string:

    +VERSION :SA818S_V1.2

Given the wide use of these devices Ampersand provides a basic configuration
capability. This avoids the need to install other SA818/SHARI configuration tools.
The configuration tab contains these settings:

![SA818](sa818-config.jpg)

Some notes:
* SHARI/SHARI-variants make **two** connections to your computer and both must 
be configured properly for the device to work:
  - SHARIs contain a CM1xx USB interface that enables audio to pass between 
the radio module and your computer. This is configured using the "Audio Device"
menu the configuration tab. The CM1xx chip also provides some I/O 
lines that can be used to receive the COS signal from your radio and to send
the PTT signal to your radio. These signals are configured using the "Carrier From" and 
"PTT To" options on the configuration tab.
  - SHARIs also contain a SA818 module that is programmed using a serial 
interface. Some SHARI vendors implement this interface using a USB port while others
implement it using GPIO pins. This serial interface is selected using the "Command Port" 
menu on the configuration tab.
* At the moment any configuration errors will be displayed on the console log. I 
will improve this in a future release.
* Many of the SHARI-type devices contain a CM1xx chip, so all of the audio setup 
described above still applies. Please test/adjust carefully to ensure good audio
on the AllStar network.
* The Command Port setting is the serial device that is used to communicate with the
SA818. Ampersand identifies this port by it's physical address (i.e. USB bus/port
numbers) to avoid some of the problems associated with multiple serial ports on the
same machine.
* I don't have any advice on setting the squelch level. 4 seems to work well in all
of my testing.
* The "Volume" setting controls the receive audio level (i.e audio out of the radio 
module and into the AllStar network). I don't have any advice on setting this level,
but 8 (highest) has worked fine in my tests. Please note that there is some coordination
required between this setting and the "Receive Mixer" setting earlier on the configuration
screen.

Discarding HID Input (Linux)
============================

(Please see [this article for more detail](https://www.florian-wolters.de/posts/discard-hid-input-from-cm108-device/))

There's an interesting problem that shows up on my computer when using
CM1XX-based radio interfaces. Per convention, most interface vendors
have connected the carrier detect (COS) signal to the **Volume Down**
pin on the CM108 chip. This is nice since it allows application software
to read the COS signal status without any additional hardware, but it can create
strange behaviors if your desktop environment thinks that you want to turn 
the audio volume down every time a carrier is detected. This behavior 
depends on your desktop configuration, but I've had this problem on Windows
and Linux.

Linux provides a way to work around this. It turns out the CM1XX volume
down pin is mapped to a keypress event to achieve the volume function.
This can be undone.

Create a custom udev hwdb rule file called /etc/udev/hwdb.d/50-cm108.hwdb.

Put this into the file, substituting the **correct** USB vendor ID/product ID:

    # Ignore HID input events from CM108 GPIOs. NOTE: Exactly one space
    # before the "KEYBOARD_KEY ..." line.
    evdev:input:b*v0D8Cp0012*
     KEYBOARD_KEY_c00ea=reserved

The "v0D8Cp0012" above refers the vendor ID/product ID of a common C-Media
USB audio interface, but **it's possible that your device will have a different
product ID**. Please use the lsusb command to validate the IDs being presented
by your hardware.

And then reload the hwdb:

    sudo systemd-hwdb update
    sudo udevadm trigger

Another Possible Solution to the "Volume Down" Problem (Linux)
==============================================================

Linux installations that include the Pulse Audio system 
may introduce a different variant of the CM1xx Volume Down 
problem. Pulse Audio often appears on systems that install a Window 
Manager (ex: KDE) since it improves the end-user experience.

I'm not a Pulse Audio expert, but it appears that:
* The Master Volume setting on the desktop is controlled by 
the volume up/down controls on the keyboard and possibly by 
the buttons on other CM1xx devices that are connected.
* When the Pulse Audio Master Volume is reduced, it also reduces
the volume on other sound devices that are under Pulse Audio control.

_(NOTE: If anyone understands this mechanism better, please reach out.)_

It's the second point above that causes the big problem. Every 
time the COS signal is asserted, the output volume on the audio 
interface is reduced. This is a frustrating problem. 

This can be resolved by telling Pulse Audio to ignore all CM1xx devices
connected to the system. There is a command-line way to force Pulse Audio to 
release all CM1xxx interfaces:

    # Make sure you have the pactl utility installed:
    sudo apt install pulseaudio-utils
    # Set profile to off for each C-Media sound card
    pactl list cards short | awk '$2 ~ /C-Media/' | awk '{print $1}' | xargs -I {} pactl set-card-profile {} off

This can also be done using to Pulse Audio control program (pavucontrol):

* Start the pavucontrol tool, which should open a sound control screen. (Note, you may need
to install this using sudo apt install pavucontrol.)
* Go to the Configuration tab.
* Find your USB audio device.
* Select the "Off" profile on the drop-down menu.

# Using Explicit Connection URIs

Normally connections are initiated using a node number. DNS is used to convert an ASL node 
number into a IP address/port number combination. In some situations it is desirable to bypass
the usual DNS lookup and provide the target address explicitly. This can be accomplished using
an IAX URI syntax like this:

    iax:radio@192.168.8.143:4568/672732,none

The tokens of the URI are as follows:

    iax:username@ip_address:port/number,password

* iax: - This is a constant
* username - Per the ASL standard, use "radio" for public nodes or the username provided by the
node operator when connecting to a private node.
* ip_address - The IP address of the target node. IPv4 only at the moment.
* port - The IAX port number of the target node. Defaults to 4569.
* number - The ASL node number of the target node.
* password - Ignored when connecting to a public node or the password provided by the node operator
when connecting to a private node.

# Using Public Key Authentication

The current AllStarLink network uses source IP address validation to authenticate callers. This method provides
reasonable security since a secret password is needed to register the association between a node
number and its IP address. Unfortunately, 
this method also creates an unnecessary link between network addressing and authentication. The problem is exacerbated by a ~15 minute latency in the current ASL registration process. This latency is particularly inconvenient in two situations:
* When a node starts up after an extended down-time. The current ASL registration server "times out" registrations that aren't refreshed constantly. Authentication of new calls will
fail until the new node number/IP address association fully propagates.
* When a mobile node moves to a different location on the network. Authentication of new calls will
fail until the new node number/IP address association fully propagates.

Ampersand supports an alternate method of authentication that completely eliminates the dependency on IP addresses.
A public-key encryption method is used to authenticate inbound calls.

Ampersand uses Ed25519 key-pairs for authentication. Ed25519 is a highly secure, fast, and modern public-key digital signature scheme that is based on elliptic curve cryptography. Importantly,
the keys are reasonably short (64 characters) which makes setup easy. The Ed25519 cryptographic 
algorithms are well suited to compute/memory-constrained microcontroller implementations.

**NOTE: At the moment this mechanism is only useful for Ampersand-to-Ampersand calls.**

This authentication mechanism relies on a few extensions to the IAX2 protocol. These extensions
have been formally proposed to [IANA](https://www.iana.org/) and they are currently under review.
You can see [the current text of the proposal here](https://github.com/brucemack/rfc5456-update/blob/main/proposal-2.md).

The one-time setup is straight-forward:
* The caller creates a public/private key pair for ASL use. 
* The caller creates a DNS subdomain in the [ampr.org domain](https://portal.ampr.org/home) and posts their ASL public key. More on this
below.
* The caller configures their Ampersand server with their private key. 
* The caller configures their Ampersand server with their amateur callsign. 

When a call is made:
* The caller places a call to an Ampersand node. The protocol includes the caller's call sign in the initial message.
* The called node uses the ASL stats API (ex: http://stats.allstarlink.org/api/stats/61057) to 
validate that the caller's call sign is associated with the node number that they are calling from.
* The called node uses the caller's call sign to check the ampr.org domain for the caller's public key.
* The called node sends back an Ed25519 authentication challenge to the caller.
* The caller signs the challenge using its private key and responds. 
* The called node uses the caller's public key to validate the signed challenge.

If no public key is found for the caller the server falls back to the traditional source IP address
validation mechanism.

## Generating Your Key Pair

You can generate the Ed25519 public/private key pair any way you want. [This online tool](https://cyphr.me/ed25519_tool/ed.html) is one easy way, but very paranoid users may not view this as sufficiently 
secure. Importantly, **the hex string representation of the public and private keys are exactly 64 characters in length.**

Here's an example key pair:

* Private: A2CF5B0FEC039AF4F3EB10294CBFBD021ADFFD3BFEAFBB48136CA3A4C5DA860B
* Public: 7F406A33164D59F6A42F15C70106F669F4199482F91B5F32A259AE795ECCEA18

## Publishing Your Public Key

Ampersand leverages the [AMPRNet system](https://portal.ampr.org/home) sponsored by the [ARDC](https://www.ardc.net/). There are a few reasons for selecting this method of key publication:
* The service is free for hams.
* The ARDC performs a license validation during the account setup process. This provides an added
level of security and ensures that only licensed hams will be able to use the ASL network.
* Many hams already have ARDC/44net accounts.

If not done already, setup an AMPRNet account by following the [registration instructions](https://portal.ampr.org/register/step-one).

Using the DNS menu on the AMPR Portal, create a subdomain for your call sign. In my case, I created a subdomain called "kc1fsz.ampr.org."

Under your subdomain, create a DNS resource record of type TXT with hostname "aslpk." The data for this
record should contain your 64-character public key in quotes. The full DNS record name will be "aslpk.YOURCALLSIGN.ampr.org."  Here's what it looks like in my case:

![Example 1](pk1.jpg)

Wait a few hours for the new record to propagate.

You can validate your public TXT record using any DNS query tool. [Here is one example of a lookup tool](https://mxtoolbox.com/txtlookup.aspx). Here's what it looks like in my case:

![Example 2](pk2.jpg)

Finally, paste your private key into the Ampersand configuration tab on your server.

![Example 3](pk3.jpg)

# Running a Private Node

A private node is not registered in the ASL directory and is not visible
on the "normal" ASL network. This capability may be useful on private IP networks
or for nodes on the public internet that should not be available to the general public.
The only way to connect to a private node is to use explicit connection parameters (i.e. 
address, port, username, password, etc.). The method of connecting to a private node from 
an Ampersand server is described in the next section.

To operate a private node you must do the following:
* Assign yourself a node number and enter it on the Ampersand configuration 
tab. Private node numbers are arbitrary but it's best not to re-use
the number of a public node to avoid confusion. Private nodes often use the
1000–1999 range.
* Leave the password field blank on the Ampersand configuration tab. This
tells your private node not to attempt to register itself into the ASL
directory.
* Create an authentication file with the username/passwords for the callers
who will be allowed to connect to your node. This file is described in the
next section. 
* Distribute the private node's number, IP address and port number to 
any authorized callers. Private node callers will also need their username and password.

Private Ampersand nodes will be able to accept connections from non-Ampersand
callers who follow the correct procedure for sending a username and password
at connection time. (Include reference to ASL documentation here)

# Accepting Private Connections

In this context, the term "private connection" means a connection from a node that is not registered
in the ASL directory. This situation most often applies to a connection to/from a private node, but 
there may be other situations where you want to support calls for node numbers that are not in the 
ASL directory. 

From what understand, the ASL convention assumes that a call using the username "radio" 
is a public connection that will be authenticated using the ASL registry. This is the default
behavior of most calls placed on the ASL network. Calls that use any other
username will treated as private connections.

There are two things that need to happen to establish a private connection:
* The called node must assign a username password for the caller to use. Remember that the ASL directory
is not used to authenticate callers in this mode.
* The caller must provide a valid username/password combination at connection time.

An Ampersand node can accept a private connection from any AllStarLink node assuming authentication
is configured properly. This feature is not limited to Ampersand-to-Ampersand calls.

The called node must create an authentication file that defines a set of username/password
combinations. The authentication file looks like this:

    # Ampersand ASL Server
    # Example authentication file
    # Bruce MacKinnon KC1FSZ 27-May-2026
    #
    # Each line contains two tokens (space delimited). The first token is the IAX 
    # username and the second token is the password.
    #
    # Blank lines and lines starting with "#" are comments
    bruce 999
    ka1iyr a8urdr0

The location of this authentication must be specified on the Configuration tab:

![Private Node 2](priv2.jpg)

# Establishing a Private Connection

As mentioned above, a private connection makes no use of the ASL registry. Therefore, a private caller
must provide call parameters explicitly. Per the [IAX2 RFC](https://datatracker.ietf.org/doc/html/rfc5456#page-8), a URI notation is used to provide these details.

When originating a call using a private connection the URI looks like this:

    iax:bruce@192.168.8.143:4568/1002,999

In this example the connection is to node 1002 at address 192.168.8.143 using port 4568. The
username/password combination used to authenticate the call is bruce/999.

IAX URIs can be entered in the node number field on the Ampersand home tab. IAX URIs can also be
used in favorites to cut down on typing.

![Private Node 1](priv1.jpg)

Asking For Help
===============

I'm happy to take any questions, but keep in mind that I'm not an expert on
Linux administration.

Please **do not** post questions that are specific to the Ampersand Server on the [AllStarLink Community Forum](https://community.allstarlink.org/). That forum
is friendly and is very useful for general AllStarLink questions, but they are
primarily focused on supporting the Asterisk/app_rpt based software.

