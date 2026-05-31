This is the repo that builds the main server that supports linking between radios and nodes on the ASL network. This project builds
on Linux (Debian 13) using arm64 or x86-64 architectures.

[Most of the Ampersand project documentation is here](https://mackinnon.info/ampersand/).

> [!IMPORTANT]
> **If you are just looking to install/run the server, you probably want to</span> [start here](https://github.com/Ampersand-ASL/amp-server/blob/main/docs/user.md)!**

> [!IMPORTANT]
> If you are using the AllStarLink system please [make a dontation](https://www.allstarlink.org/about/donate.php) to support the network. 

To understand the structure of the server, the best place to start 
is [main.cpp](https://github.com/Ampersand-ASL/amp-server/blob/main/src/main.cpp).

Most of what is on the rest of this page is relevant to development. The
[normal installation/user instructions are here](https://github.com/Ampersand-ASL/amp-server/blob/main/docs/user.md).

# One-Time Developer Machine Setup (Linux)

Make a keypair if necessary:

        # No passphrase used
        ssh-keygen -t ed25519 -b 4096 -N ''

Get the public SSH key loaded onto the machine to enable login, remote editing, etc.

        cd .ssh
        echo "ssh-ed25519 <PUBLIC_SSH_KEY> user@host" >> authorized_keys

# Building The Server From Source (Linux)

Install the prerequisites:

    sudo apt update
    sudo apt -y upgrade
    sudo apt -y install wget emacs-nox git cmake build-essential git xxd libasound2-dev libcurl4-gnutls-dev Libusb-1.0-0-dev gdb 

Tell git to retain credentials (insecure):

        git config --global credential.helper store

Get the code and build:

    git clone https://github.com/Ampersand-ASL/amp-server.git
    cd amp-server
    git config pull.rebase false
    git submodule update --init
    cmake -B build
    cmake --build build 
    
# Packaging the Build (Linux)

    # Update version number in this file
    # Update CHANGELOG.md
    # Update version in main.cpp
    # Update version in docs/user.md
    export AMP_SERVER_VERSION=20260528
    export AMP_ARCH=$(uname -m)
    scripts/make-package.sh        
    # Move as needed
    scp bruce@pi5:/tmp/amp-${AMP_SERVER_VERSION}-${AMP_ARCH}.tar.gz .
    # And them move the .tar.gz to the Ampersand S3 bucket

# (Debug) Getting Line Number From Stack Trace

        addr2line -e ./amp-server -fC 0x138a0

# Code Metrics

        cloc --vcs=git --exclude-list-file=.clocignore .



# Wireguard Stuff

bruce@pi5:~ $ sudo wg-quick up wg0
[#] ip link add wg0 type wireguard
[#] wg setconf wg0 /dev/fd/63
[#] ip -4 address add 44.27.134.170/32 dev wg0
[#] ip -6 address add fe80::3bb4:112d:8f55:127f/128 dev wg0
[#] ip link set mtu 1380 up dev wg0
[#] resolvconf -a tun.wg0 -m 0 -x
/etc/resolvconf/update.d/libc: Warning: /etc/resolv.conf is not a symbolic link to /run/resolvconf/resolv.conf
[#] wg set wg0 fwmark 51820
[#] ip -6 route add ::/0 dev wg0 table 51820
[#] ip -6 rule add not fwmark 51820 table 51820
[#] ip -6 rule add table main suppress_prefixlength 0
[#] nft -f /dev/fd/63
[#] ip -4 route add 0.0.0.0/0 dev wg0 table 51820

# ShariPi Stuff

* Enabled serial hardware using rapsi-config. Got the serial port at /dev/ttyAMA0.

# Resolving CM108 Volume Down/COS Conflict

        sudo apt install pulseaudio-utils
        pactl list cards short
        pactl set-card-profile 48 off

        # Find C-Media Cards
        pactl list cards short | awk '$2 ~ /C-Media/' | awk '{print $1}'

        # Set profile to off for each C-Media sound card
        pactl list cards short | awk '$2 ~ /C-Media/' | awk '{print $1}' | xargs -I {} pactl set-card-profile {} off




