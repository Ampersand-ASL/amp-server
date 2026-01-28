This is the repo that builds the main server that supports linking between radios and nodes on the ASL network. This project builds
on Linux (Debian 13) using arm64 or x86-64 architectures.

[Most of the documentation is here](https://mackinnon.info/ampersand/).

To understand the structure of the server, the best place to start 
is [main.cpp](https://github.com/Ampersand-ASL/amp-server/blob/main/src/main.cpp).

**MORE SOFTWARE STRUCTURE DOCUMENTATION TO FOLLOW**

# One-Time Machine Setup (To Run the Server)

Make a keypair if necessary:

        # No passphrase used
        ssh-keygen -t ed25519 -b 4096 -N ''

Get the public SSH key loaded onto the machine to enable login, remote editing, etc.

        cd .ssh
        echo "ssh-ed25519 <PUBLIC_SSH_KEY> user@host" >> authorized_keys

Tell git to retain credentials (insecure):

        git config --global credential.helper store

An adjustment needs to be made to allow a normal user to access the HID interfaces:

Create /etc/udev/rules.d/99-mydevice.rules with this contents:

        # C-Media Vendor ID
        SUBSYSTEM=="hidraw", ATTRS{idVendor}=="0d8c", MODE="0666", TAG+="uaccess"

(And include any other devices you plan to use)

Force reload of rules:

        sudo udevadm control --reload-rules
        sudo udevadm trigger

Install the server:

        export AMP_SERVER_VERSION=20260128
        export AMP_ARCH=$(uname -m)
        wget https://ampersand-asl.s3.us-west-1.amazonaws.com/releases/amp-${AMP_SERVER_VERSION}-${AMP_ARCH}.tar.gz
        tar xvf tar xvf amp-${AMP_SERVER_VERSION}-${AMP_ARCH}.tar.gz
        ln -s amp-$AMP_SERVER_VERSION-${AMP_ARCH} amp       

# Building The Server From Source

Install the prerequisites:

    sudo apt install cmake build-essential git xxd libasound2-dev libcurl4-gnutls-dev Libusb-1.0-0-dev

Get the code and build:

    git clone https://github.com/Ampersand-ASL/amp-server.git
    cd amp-server
    git submodule update --init
    cmake -B build
    cmake --build build 
    
# Packaging the Build

    export AMP_SERVER_VERSION=20260128
    export AMP_ARCH=$(uname -m)
    scripts/make-package.sh        
    # Move as needed
    scp /tmp/amp-${AMP_SERVER_VERSION}-${AMP_ARCH}.tar.gz bruce@pi5:/tmp
    # And them move the .tar.gz to the Ampersand S3 bucket

# (Debug) Getting Line Number From Stack Trace

        addr2line -e ./amp-server -fC 0x138a0

# Code Metrics

        cloc --vcs=git --exclude-list-file=.clocignore .