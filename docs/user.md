Installation Instructions
=========================

Install required packages:

    sudo apt install wget net-tools

An adjustment needs to be made to allow a normal user to access the HID interfaces. Create /etc/udev/rules.d/99-mydevice.rules with this contents:

    SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0d8c", MODE="0666", TAG+="uaccess"

(And include any other devices you plan to use)

Reboot, or just force reload of rules:

    sudo udevadm control --reload-rules
    sudo udevadm trigger

Get the latest install package:

    wget https://mackinnon.info/ampersand/releases/amp-20260106-x86_64.tar.gz
    tar xvf amp-20260106-x86_64.tar.gz
    ln -s amp-20260106-x86_64 amp

Running the Server
==================

    cd amp
    amp-server 

Command-line options:

* --httport (defaults to 8080).  Used to change the port that the web UI runs on.
* --config (defaults $HOME/amp-server.json). Used to change the location of the configuration 
file.
* --trace Used to turn on extended network tracing.

Configuration
=============




    

