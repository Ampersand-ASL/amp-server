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

# ----- Make resource objects ------------------------------------------------

# xxd -n _www_index_html -i ../amp-core/www/index.html resource-amp-core-www-index.c

# Converting an html file into a .c file. The temporary .c file is 
# created in the build directory.
set(RESOURCE0 "${CMAKE_CURRENT_SOURCE_DIR}/amp-core/www/index.html")
set(OBJECT0 "${CMAKE_CURRENT_BINARY_DIR}/resource-amp-core-www-index.c")
set(NAME0 "_amp_core_www_index_html")
add_custom_command(
    OUTPUT ${OBJECT0}
    COMMAND xxd
    ARGS 
        -n ${NAME0}
        -i 
        ${RESOURCE0}
        ${OBJECT0}
    DEPENDS ${RESOURCE0}
)

set(RESOURCE1 "${CMAKE_CURRENT_SOURCE_DIR}/amp-core/www/config.html")
set(OBJECT1 "${CMAKE_CURRENT_BINARY_DIR}/resource-amp-core-www-config.c")
set(NAME1 "_amp_core_www_config_html")
add_custom_command(
    OUTPUT ${OBJECT1}
    COMMAND xxd
    ARGS 
        -n ${NAME1}
        -i 
        ${RESOURCE1}
        ${OBJECT1}
    DEPENDS ${RESOURCE1}
)

# ----- amp-server ----------------------------------------------------------------

add_executable(amp-server
  src/main.cpp
  amp-core/src/service-thread.cpp
  amp-core/src/EventLoop.cpp
  amp-core/src/Message.cpp
  amp-core/src/Line.cpp
  amp-core/src/LineRadio.cpp
  amp-core/src/LineUsb.cpp
  amp-core/src/LineUsb2.cpp
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
  amp-core/src/NRLog.cpp
  amp-core/src/TraceLog.cpp
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
  sound-map/src/sound-map.cpp
  sound-map/src/sound-map-2.cpp
  sound-map/src/usb-dir.c
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
# Here is where the resources are added to the binary
  ${OBJECT0}
  ${OBJECT1}
)

target_include_directories(amp-server PRIVATE src)
target_include_directories(amp-server PRIVATE kc1fsz-tools-cpp/include)
target_include_directories(amp-server PRIVATE itu-g711-codec/src)
target_include_directories(amp-server PRIVATE cmsis-dsp-mock/include)
target_include_directories(amp-server PRIVATE amp-core/include)
# TODO: REMOVE
target_include_directories(amp-server PRIVATE amp-core/src)
target_include_directories(amp-server PRIVATE sound-map/include)
target_include_directories(amp-server PRIVATE ed25519/src)
target_include_directories(amp-server PRIVATE kc1fsz-sdrc/sw/src)
target_include_directories(amp-server PRIVATE kc1fsz-sdrc/sw/include)
target_include_directories(amp-server PRIVATE cpp-httplib)
target_include_directories(amp-server PRIVATE json/include)
target_include_directories(amp-server PRIVATE argparse/include)

target_link_libraries(amp-server -lasound -lcurl -lusb-1.0)

# ----- hello-http-server -------------------------------------------------

add_executable(hello-http-server EXCLUDE_FROM_ALL
  src/demos/hello-http-server.cpp
  kc1fsz-tools-cpp/src/linux/StdClock.cpp
)
target_include_directories(hello-http-server PRIVATE src)
target_include_directories(hello-http-server PRIVATE include)
target_include_directories(hello-http-server PRIVATE kc1fsz-tools-cpp/include)
target_include_directories(hello-http-server PRIVATE cpp-httplib)
target_include_directories(hello-http-server PRIVATE json/include)

# ----- hello-mixer -------------------------------------------------------

add_executable(hello-mixer EXCLUDE_FROM_ALL
  src/demos/hello-mixer.cpp
  amp-core/src/LineUsb2.cpp
  kc1fsz-tools-cpp/src/linux/StdClock.cpp
)
target_include_directories(hello-mixer PRIVATE src)
target_include_directories(hello-mixer PRIVATE include)
target_include_directories(hello-mixer PRIVATE cmsis-dsp-mock/include)
target_include_directories(hello-mixer PRIVATE amp-core/include)
target_include_directories(hello-mixer PRIVATE amp-core/src)
target_include_directories(hello-mixer PRIVATE kc1fsz-tools-cpp/include)
target_include_directories(hello-mixer PRIVATE kc1fsz-sdrc/sw/src)
target_include_directories(hello-mixer PRIVATE kc1fsz-sdrc/sw/include)
target_include_directories(hello-mixer PRIVATE cpp-httplib)
target_link_libraries(hello-mixer -lasound -lusb-1.0)

