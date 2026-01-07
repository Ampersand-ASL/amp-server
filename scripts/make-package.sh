#!/bin/bash
VERSION=$AMP_SERVER_VERSION
ARCHITECTURE=$(uname -m)
mkdir -p /tmp/amp-$VERSION-$ARCHITECTURE
rm -rf /tmp/amp-$VERSION-$ARCHITECTURE/*
cp amp-server /tmp/amp-$VERSION-$ARCHITECTURE
cd /tmp
tar -czf /tmp/amp-$VERSION-$ARCHITECTURE.tar.gz amp-$VERSION-$ARCHITECTURE
