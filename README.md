# Ampersand Server

The core server supports linking between radios and nodes on the ASL network.

# One-Time Machine Setup (To Run the Server)

Get the public SSH key loaded onto the machine to enable login, remote editing, etc.

        cd .ssh
        echo "ssh-ed25519 <PUBLIC_SSH_KEY> user@host" >> authorized_keys

Tell git to retain credentials:

        git config --global credential.helper store

An adjustment needs to be made to allow a normal user to access the HID interfaces:

Create /etc/udev/rules.d/99-mydevice.rules with this contents:

        SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0d8c", MODE="0666", TAG+="uaccess"
        SUBSYSTEM=="hidraw", ATTRS{idVendor}=="046d", ATTRS{idProduct}=="0ade", MODE="0666", TAG+="uaccess"

(And include any other devices you plan to use)

Force reload of rules:

        sudo udevadm control --reload-rules
        sudo udevadm trigger

# Building The Server

    sudo apt install xxd libasound2-dev libcurl4-gnutls-dev
    git clone https://github.com/Ampersand-ASL/amp-server.git
    cd amp-server
    git submodule update --init
    mkdir build
    cd build
    cmake ..
    make

# Packaging

    export AMP_SERVER_VERSION=20260106
    ../scripts/make-package.sh        

# Binary Install

# (Debug) Getting Line Number From Stack Trace

        addr2line -e ./amp-server -fC 0x138a0
        