# Ampersand Project
# Copyright (C) 2025, Bruce MacKinnon KC1FSZ
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# ( at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
cmake_minimum_required(VERSION 3.13)
project(ampersand C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
add_compile_options(-fstack-protector-all -Wall -Wpedantic -ggdb -O0)

# Set the default install prefix (e.g., /usr/local on Linux)
#set(CMAKE_INSTALL_PREFIX "/tmp")

add_executable(main
  src/main.cpp
  amp-core/src/service-thread.cpp
  amp-core/src/EventLoop.cpp
  amp-core/src/Message.cpp
  amp-core/src/Line.cpp
  amp-core/src/LineRadio.cpp
  amp-core/src/LineUsb.cpp
  amp-core/src/LineIAX2.cpp
  amp-core/src/IAX2FrameFull.cpp
  amp-core/src/IAX2Util.cpp
  amp-core/src/RegisterTask.cpp
  amp-core/src/StatsTask.cpp
  amp-core/src/ManagerTask.cpp
  amp-core/src/RetransmissionBufferStd.cpp
  amp-core/src/Transcoder_G711_ULAW.cpp
  amp-core/src/Transcoder_SLIN_48K.cpp
  amp-core/src/Transcoder_SLIN_16K.cpp
  amp-core/src/Bridge.cpp
  amp-core/src/BridgeCall.cpp
  amp-core/src/BridgeIn.cpp
  amp-core/src/BridgeOut.cpp
  amp-core/src/Resampler.cpp
  amp-core/src/ConfigPoller.cpp
  amp-core/src/MultiRouter.cpp
  amp-core/src/WebUi.cpp
  amp-core/src/SignalIn.cpp
  amp-core/src/ThreadUtil.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/NetUtils.cpp
  kc1fsz-tools-cpp/src/DTMFDetector2.cpp
  kc1fsz-tools-cpp/src/MicroDNS.cpp
  kc1fsz-tools-cpp/src/StdPollTimer.cpp
  kc1fsz-tools-cpp/src/linux/StdClock.cpp
  kc1fsz-tools-cpp/src/linux/MTLog.cpp
  kc1fsz-tools-cpp/src/md5/md5c.c
  itu-g711-codec/src/codec.cpp
  itu-g711-codec/src/Plc.cpp
  cmsis-dsp-mock/src/main.cpp
  sound-map/src/sound-map.c
  sound-map/src/usb-dir.c
  sound-map/src/sound-map-2.cpp
  ed25519/src/add_scalar.c
  ed25519/src/ge.c
  ed25519/src/keypair.c
  ed25519/src/seed.c
  ed25519/src/sign.c
  ed25519/src/fe.c
  ed25519/src/key_exchange.c
  ed25519/src/sc.c
  ed25519/src/sha512.c
  ed25519/src/verify.c
)

target_include_directories(main PRIVATE src)
target_include_directories(main PRIVATE kc1fsz-tools-cpp/include)
target_include_directories(main PRIVATE itu-g711-codec/src)
target_include_directories(main PRIVATE cmsis-dsp-mock/include)
target_include_directories(main PRIVATE amp-core/include)
# TODO: REMOVE
target_include_directories(main PRIVATE amp-core/src)
target_include_directories(main PRIVATE sound-map/include)
target_include_directories(main PRIVATE ed25519/src)
target_include_directories(main PRIVATE kc1fsz-sdrc/sw/src)
target_include_directories(main PRIVATE kc1fsz-sdrc/sw/include)
target_include_directories(main PRIVATE cpp-httplib)
target_include_directories(main PRIVATE json/include)
target_include_directories(main PRIVATE argparse/include)

target_link_libraries(main -lasound -lcurl -lusb-1.0)

# ----- hello-http-server -------------------------------------------------

add_executable(hello-http-server
  src/demos/hello-http-server.cpp
  kc1fsz-tools-cpp/src/linux/StdClock.cpp
)
target_include_directories(hello-http-server PRIVATE src)
target_include_directories(hello-http-server PRIVATE include)
target_include_directories(hello-http-server PRIVATE kc1fsz-tools-cpp/include)
target_include_directories(hello-http-server PRIVATE cpp-httplib)
target_include_directories(hello-http-server PRIVATE json/include)
