Installation Instructions
=========================

Install required packages:

    sudo apt install wget net-tools libcurl4-gnutls-dev

An adjustment needs to be made to allow non-root users to access the HID interfaces. Create /etc/udev/rules.d/99-mydevice.rules with this contents:

    # The C-Media vendor ID
    SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0d8c", MODE="0666", TAG+="uaccess"

(And include any other devices you plan to use)

Reboot, or just force reload of rules:

    sudo udevadm control --reload-rules
    sudo udevadm trigger

Get the latest install package:

    wget https://mackinnon.info/ampersand/releases/amp-20260109-x86_64.tar.gz
    tar xvf amp-20260109-x86_64.tar.gz
    ln -s amp-20260100-x86_64 amp

Running the Server
==================

    cd amp
    ./amp-server 

Command-line options:

* --httport (defaults to 8080).  Used to change the port that the web UI runs on.
* --config (defaults $HOME/amp-server.json). Used to change the location of the configuration 
file.
* --trace Used to turn on extended network tracing.

Setup/Configuration
===================

Point your browser to the server using port 8080 (the default), or a different port if you
have configured one.  The main screen will look like this:

![Amp1](amp-1.jpg)

Press the "Configuration" link at the bottom of the screen to get to the configuration screen
that looks like this:

![Amp2](amp-2.jpg)

Fill in your node number and password. All other defaults should be enough to get your started.

The audio levels will be the first thing to configure. Your audio level will be displayed in 
the system log any time you key your transmitter (regardless of whether you are connected to 
any other nodes). The levels will be displayed like this:

![Amp3](amp-3.jpg)

Things That Aren't Enabled Yet
==============================

* DTMF pad
* CTCSS/PTT functionality
* Repeater functionality
* List of linked nodes for each node
* More status messages need to be shown on the main page