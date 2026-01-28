This is the repo that builds the main server that supports linking between radios and nodes on the ASL network. This project builds
on Linux (Debian 13) using arm64 or x86-64 architectures.

[Most of the documentation is here](https://mackinnon.info/ampersand/).

To understand the structure of the server, the best place to start 
is [main.cpp](https://github.com/Ampersand-ASL/amp-server/blob/main/src/main.cpp).

Most of what is on this page is relevant to development. The
[normal installation/user instructions are here](https://github.com/Ampersand-ASL/amp-server/blob/main/docs/user.md).

 <span style="color:red">**If you are just looking to install/run the server, you probably want to [start here](https://github.com/Ampersand-ASL/amp-server/blob/main/docs/user.md)!**</span>

# One-Time Developer Machine Setup 

Make a keypair if necessary:

        # No passphrase used
        ssh-keygen -t ed25519 -b 4096 -N ''

Get the public SSH key loaded onto the machine to enable login, remote editing, etc.

        cd .ssh
        echo "ssh-ed25519 <PUBLIC_SSH_KEY> user@host" >> authorized_key

# Building The Server From Source

Install the prerequisites:

    sudo apt install cmake build-essential git xxd libasound2-dev libcurl4-gnutls-dev Libusb-1.0-0-dev

Tell git to retain credentials (insecure):

        git config --global credential.helper store

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