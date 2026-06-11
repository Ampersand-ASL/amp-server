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
add_compile_options(-fstack-protector-all -Wall -Wpedantic -g -O0)

# ==============================================================================
# NEEDED FOR CPPCHCEK
#find_program(CPPCHECK_PATH cppcheck)
#if(CPPCHECK_PATH)
#    set(CMAKE_CXX_CPPCHECK ${CPPCHECK_PATH} "--enable=all" "--inconclusive")
#endif()
# ==============================================================================

# ----- Make resource objects ------------------------------------------------

# xxd -n _www_index_html -i ../amp-core/www/index.html resource-amp-core-www-index.c

# Converting an html file into a .c file. The temporary .c file is 
# created in the build directory.
set(RESOURCE0 "${CMAKE_CURRENT_SOURCE_DIR}/amp-core/www/index.html")
set(OBJECT0 "${CMAKE_CURRENT_BINARY_DIR}/resource-amp-core-www-index-html.c")
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
set(OBJECT1 "${CMAKE_CURRENT_BINARY_DIR}/resource-amp-core-www-config-html.c")
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

set(RESOURCE2 "${CMAKE_CURRENT_SOURCE_DIR}/amp-core/www/main.css")
set(OBJECT2 "${CMAKE_CURRENT_BINARY_DIR}/resource-amp-core-www-main-css.c")
set(NAME2 "_amp_core_www_main_css")
add_custom_command(
    OUTPUT ${OBJECT2}
    COMMAND xxd
    ARGS 
        -n ${NAME2}
        -i 
        ${RESOURCE2}
        ${OBJECT2}
    DEPENDS ${RESOURCE2}
)

# Converting an html file into a .c file. The temporary .c file is 
# created in the build directory.
set(RESOURCE3 "${CMAKE_CURRENT_SOURCE_DIR}/amp-core/www/log.html")
set(OBJECT3 "${CMAKE_CURRENT_BINARY_DIR}/resource-amp-core-www-log-html.c")
set(NAME3 "_amp_core_www_log_html")
add_custom_command(
    OUTPUT ${OBJECT3}
    COMMAND xxd
    ARGS 
        -n ${NAME3}
        -i 
        ${RESOURCE3}
        ${OBJECT3}
    DEPENDS ${RESOURCE3}
)

# ----- amp-server ----------------------------------------------------------------

add_executable(amp-server
  src/main.cpp
  src/config-handler.cpp
  src/service-thread.cpp
  src/LocalAuthenticatorStd.cpp
  amp-core/src/EventLoop.cpp
  amp-core/src/Message.cpp
  amp-core/src/Line.cpp
  amp-core/src/LineRadio.cpp
  amp-core/src/LineUsb.cpp
  amp-core/src/LineUsb2.cpp
  amp-core/src/LineIAX2.cpp
  amp-core/src/LineIAX2_base.cpp
  amp-core/src/LineParrot.cpp
  amp-core/src/LineSDRC.cpp
  amp-core/src/IAX2FrameFull.cpp
  amp-core/src/IAX2Util.cpp
  amp-core/src/RegisterTask.cpp
  amp-core/src/StatsTask.cpp
  amp-core/src/ManagerTask.cpp
  amp-core/src/Transcoder_G711_ULAW.cpp
  amp-core/src/Transcoder_SLIN_48K.cpp
  amp-core/src/Transcoder_SLIN_16K.cpp
  amp-core/src/Transcoder_SLIN_8K.cpp
  amp-core/src/Transcoder_G726.cpp
  amp-core/src/Bridge.cpp
  amp-core/src/BridgeCall.cpp
  amp-core/src/BridgeIn.cpp
  amp-core/src/BridgeOut.cpp
  amp-core/src/ProgramUtils.cpp
  amp-core/src/Resampler.cpp
  amp-core/src/ConfigPoller.cpp
  amp-core/src/MultiRouter.cpp
  amp-core/src/WebUi.cpp
  amp-core/src/WebUi_2.cpp
  amp-core/src/SignalIn.cpp
  amp-core/src/SignalOut.cpp
  amp-core/src/ThreadUtil.cpp
  amp-core/src/TraceLog.cpp
  amp-core/src/SerialUtil.cpp
  amp-core/src/KerchunkFilter.cpp
  amp-core/src/TTSServiceSimple.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/NetUtils.cpp
  kc1fsz-tools-cpp/src/DTMFDetector2.cpp
  kc1fsz-tools-cpp/src/MicroDNS.cpp
  kc1fsz-tools-cpp/src/StdPollTimer.cpp
  kc1fsz-tools-cpp/src/fixed_math.cpp
  kc1fsz-tools-cpp/src/TaggedBuffer.cpp
  kc1fsz-tools-cpp/src/StateMachine.cpp
  kc1fsz-tools-cpp/src/linux/StdClock2.cpp
  kc1fsz-tools-cpp/src/linux/MTLog.cpp
  kc1fsz-tools-cpp/src/linux/FileUtils.cpp
  kc1fsz-tools-cpp/src/md5/md5c.c
  kc1fsz-tools-cpp/src/crc/crc.c
  kc1fsz-sdrc/sw/src/DigitalAudioPortRxHandler.cpp
  cobs-c/cobs.c
  itu-g711-codec/src/codec.cpp
  itu-g711-codec/src/Plc.cpp
  cmsis-dsp-mock/src/main.cpp
  cmsis-dsp-mock/src/main-float.cpp
  sound-map/src/sound-map.cpp
  sound-map/src/sound-map-2.cpp
  sound-map/src/usb-dir.c
  sound-map/src/serial-map.cpp
  base64.c/base64.c
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
  g726-codec/src/g711.c
  g726-codec/src/g72x.c
  g726-codec/src/g726_32.c
  # Here is where the WWW resources are added to the binary
  ${OBJECT0}
  ${OBJECT1}
  ${OBJECT2}
  ${OBJECT3}
  # Audio clips
  amp-core/clips/resource_amp_core_clips_clip_0_raw.c
  amp-core/clips/resource_amp_core_clips_clip_0_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_1_raw.c
  amp-core/clips/resource_amp_core_clips_clip_1_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_2_raw.c
  amp-core/clips/resource_amp_core_clips_clip_2_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_3_raw.c
  amp-core/clips/resource_amp_core_clips_clip_3_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_4_raw.c
  amp-core/clips/resource_amp_core_clips_clip_4_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_5_raw.c
  amp-core/clips/resource_amp_core_clips_clip_5_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_6_raw.c
  amp-core/clips/resource_amp_core_clips_clip_6_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_7_raw.c
  amp-core/clips/resource_amp_core_clips_clip_7_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_8_raw.c
  amp-core/clips/resource_amp_core_clips_clip_8_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_9_raw.c
  amp-core/clips/resource_amp_core_clips_clip_9_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_calling_node_raw.c
)
# NOTE: Compiler optimizations are turned on
target_compile_options(amp-server PRIVATE -Wall -Wpedantic -g -O3 -mtune=native)

target_include_directories(amp-server PRIVATE 
  src
  kc1fsz-tools-cpp/include
  kc1fsz-tools-cpp/include/kc1fsz-tools/crc
  itu-g711-codec/src
  cmsis-dsp-mock/include
  cobs-c
  amp-core/include
  amp-core/src
  sound-map/include
  ed25519/src
  kc1fsz-sdrc/sw/src
  kc1fsz-sdrc/sw/include
  cpp-httplib
  base64.c
  json/include
  argparse/include
  g726-codec/src
  )

target_link_libraries(amp-server -lasound -lcurl -lusb-1.0)

# CPPCHECK
#set_target_properties(amp-server PROPERTIES CXX_CPPCHECK "${CPPCHECK_PATH};--enable=warning")

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

# ------ cobs-test-1 ---------------------------------------------------------

add_executable(cobs-test-1 EXCLUDE_FROM_ALL
  src/tests/cobs-test-1.cpp
  kc1fsz-tools-cpp/src/NetUtils.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  cobs-c/cobs.c
) 

target_include_directories(cobs-test-1 PRIVATE src)
target_include_directories(cobs-test-1 PRIVATE kc1fsz-tools-cpp/include)
target_include_directories(cobs-test-1 PRIVATE cobs-c)

# ------ sdrc-msg-test-1 ---------------------------------------------------

add_executable(sdrc-msg-test-1 EXCLUDE_FROM_ALL
  src/tests/sdrc-msg-test-1.cpp
  kc1fsz-sdrc/sw/src/DigitalAudioPortRxHandler.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/crc/crc.c
  cobs-c/cobs.c
) 

target_include_directories(sdrc-msg-test-1 PRIVATE src)
target_include_directories(sdrc-msg-test-1 PRIVATE kc1fsz-sdrc/sw/src)
target_include_directories(sdrc-msg-test-1 PRIVATE kc1fsz-tools-cpp/include)
target_include_directories(sdrc-msg-test-1 PRIVATE kc1fsz-tools-cpp/include/kc1fsz-tools/crc)
target_include_directories(sdrc-msg-test-1 PRIVATE cobs-c)

# ----- hello-audio ----------------------------------------------------------------

add_executable(hello-audio EXCLUDE_FROM_ALL
  src/demos/hello-audio.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/StdPollTimer.cpp
  kc1fsz-tools-cpp/src/linux/StdClock.cpp
  sound-map/src/sound-map.cpp
  sound-map/src/sound-map-2.cpp
  sound-map/src/usb-dir.c
)
# NOTE: Compiler optimizations are turned on
target_compile_options(hello-audio PRIVATE -Wall -Wpedantic -g -O3 -mtune=native)

target_include_directories(hello-audio PRIVATE 
  src
  kc1fsz-tools-cpp/include
  sound-map/include)

target_link_libraries(hello-audio -lasound -lusb-1.0)

# ----- hello-audio-2 ----------------------------------------------------------------

add_executable(hello-audio-2 EXCLUDE_FROM_ALL
  src/demos/hello-audio-2.cpp
  amp-core/src/EventLoop.cpp
  amp-core/src/Message.cpp
  amp-core/src/Line.cpp
  amp-core/src/LineRadio.cpp
  amp-core/src/LineUsb.cpp
  amp-core/src/LineUsb2.cpp
  amp-core/src/Transcoder_G711_ULAW.cpp
  amp-core/src/Transcoder_SLIN_48K.cpp
  amp-core/src/Transcoder_SLIN_16K.cpp
  amp-core/src/Transcoder_SLIN_8K.cpp
  amp-core/src/Resampler.cpp
  amp-core/src/MultiRouter.cpp
  amp-core/src/tests/TestAudioGenerator.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/StdPollTimer.cpp
  kc1fsz-tools-cpp/src/linux/StdClock.cpp
  kc1fsz-tools-cpp/src/DTMFDetector2.cpp
  sound-map/src/sound-map.cpp
  sound-map/src/sound-map-2.cpp
  sound-map/src/usb-dir.c
  itu-g711-codec/src/codec.cpp
  itu-g711-codec/src/Plc.cpp
  cmsis-dsp-mock/src/main.cpp
  cmsis-dsp-mock/src/main-float.cpp
)
# NOTE: Compiler optimizations are turned on
target_compile_options(hello-audio-2 PRIVATE -Wall -Wpedantic -g -O3 -mtune=native)

target_include_directories(hello-audio-2 PRIVATE 
  src
  amp-core/include
  amp-core/src
  kc1fsz-sdrc/sw/src
  kc1fsz-sdr
  c/sw/include
  kc1fsz-tools-cpp/include
  sound-map/include
  itu-g711-codec/src
  cmsis-dsp-mock/include
  )

target_link_libraries(hello-audio-2 -lasound -lusb-1.0)

# ----- syslog-1 

add_executable(syslog-1 EXCLUDE_FROM_ALL
  src/demos/syslog-1.cpp
  kc1fsz-tools-cpp/src/Common.cpp
) 

target_include_directories(syslog-1 PRIVATE 
  src
  kc1fsz-tools-cpp/include
  )

# ----- hello-reg ----------------------------------------------------------------

add_executable(hello-reg EXCLUDE_FROM_ALL
  src/demos/hello-reg.cpp
  amp-core/src/EventLoop.cpp
  amp-core/src/RegisterTaskIAX2.cpp
  amp-core/src/IAX2FrameFull.cpp
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/NetUtils.cpp
  kc1fsz-tools-cpp/src/StdPollTimer.cpp
  kc1fsz-tools-cpp/src/linux/StdClock.cpp
  kc1fsz-tools-cpp/src/StateMachine.cpp
  kc1fsz-tools-cpp/src/md5/md5c.c
)
# NOTE: Compiler optimizations are turned on
target_compile_options(hello-reg PRIVATE -Wall -Wpedantic -g -O3 -mtune=native)

target_include_directories(hello-reg PRIVATE 
  src
  amp-core/include
  amp-core/src
  kc1fsz-tools-cpp/include
  )

# ----- unit-test ----------------------------------------------------------------

add_executable(unit-test EXCLUDE_FROM_ALL
  src/tests/unit-test-1.cpp
  src/LocalAuthenticatorStd.cpp
  src/config-handler.cpp
  amp-core/src/EventLoop.cpp
  amp-core/src/Message.cpp
  amp-core/src/Line.cpp
  amp-core/src/LineRadio.cpp
  amp-core/src/LineUsb.cpp
  amp-core/src/LineUsb2.cpp
  amp-core/src/LineIAX2.cpp
  amp-core/src/LineIAX2_base.cpp
  amp-core/src/LineParrot.cpp
  amp-core/src/LineSDRC.cpp
  amp-core/src/IAX2FrameFull.cpp
  amp-core/src/IAX2Util.cpp
  amp-core/src/RegisterTask.cpp
  amp-core/src/StatsTask.cpp
  amp-core/src/ManagerTask.cpp
  amp-core/src/Transcoder_G711_ULAW.cpp
  amp-core/src/Transcoder_SLIN_48K.cpp
  amp-core/src/Transcoder_SLIN_16K.cpp
  amp-core/src/Transcoder_SLIN_8K.cpp
  amp-core/src/Transcoder_G726.cpp
  amp-core/src/Bridge.cpp
  amp-core/src/BridgeCall.cpp
  amp-core/src/BridgeIn.cpp
  amp-core/src/BridgeOut.cpp
  amp-core/src/ProgramUtils.cpp
  amp-core/src/Resampler.cpp
  amp-core/src/ConfigPoller.cpp
  amp-core/src/MultiRouter.cpp
  amp-core/src/WebUi.cpp
  amp-core/src/WebUi_2.cpp
  amp-core/src/SignalIn.cpp
  amp-core/src/SignalOut.cpp
  amp-core/src/ThreadUtil.cpp
  amp-core/src/TraceLog.cpp
  amp-core/src/SerialUtil.cpp
  amp-core/src/KerchunkFilter.cpp
  amp-core/src/TTSServiceSimple.cpp
  # Here is where the WWW resources are added to the binary
  ${OBJECT0}
  ${OBJECT1}
  ${OBJECT2}
  # Audio clips
  amp-core/clips/resource_amp_core_clips_clip_0_raw.c
  amp-core/clips/resource_amp_core_clips_clip_0_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_1_raw.c
  amp-core/clips/resource_amp_core_clips_clip_1_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_2_raw.c
  amp-core/clips/resource_amp_core_clips_clip_2_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_3_raw.c
  amp-core/clips/resource_amp_core_clips_clip_3_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_4_raw.c
  amp-core/clips/resource_amp_core_clips_clip_4_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_5_raw.c
  amp-core/clips/resource_amp_core_clips_clip_5_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_6_raw.c
  amp-core/clips/resource_amp_core_clips_clip_6_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_7_raw.c
  amp-core/clips/resource_amp_core_clips_clip_7_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_8_raw.c
  amp-core/clips/resource_amp_core_clips_clip_8_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_9_raw.c
  amp-core/clips/resource_amp_core_clips_clip_9_end_raw.c
  amp-core/clips/resource_amp_core_clips_clip_calling_node_raw.c
  kc1fsz-tools-cpp/src/Common.cpp
  kc1fsz-tools-cpp/src/NetUtils.cpp
  kc1fsz-tools-cpp/src/DTMFDetector2.cpp
  kc1fsz-tools-cpp/src/MicroDNS.cpp
  kc1fsz-tools-cpp/src/StdPollTimer.cpp
  kc1fsz-tools-cpp/src/fixed_math.cpp
  kc1fsz-tools-cpp/src/TaggedBuffer.cpp
  kc1fsz-tools-cpp/src/linux/StdClock.cpp
  kc1fsz-tools-cpp/src/linux/MTLog.cpp
  kc1fsz-tools-cpp/src/md5/md5c.c
  kc1fsz-tools-cpp/src/crc/crc.c
  kc1fsz-sdrc/sw/src/DigitalAudioPortRxHandler.cpp
  cobs-c/cobs.c
  itu-g711-codec/src/codec.cpp
  itu-g711-codec/src/Plc.cpp
  cmsis-dsp-mock/src/main.cpp
  cmsis-dsp-mock/src/main-float.cpp
  sound-map/src/sound-map.cpp
  sound-map/src/sound-map-2.cpp
  sound-map/src/usb-dir.c
  sound-map/src/serial-map.cpp
  base64.c/base64.c
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
  g726-codec/src/g711.c
  g726-codec/src/g72x.c
  g726-codec/src/g726_32.c
)
# NOTE: Compiler optimizations are turned on
target_compile_options(unit-test PRIVATE -Wall -Wpedantic -g -O3 -mtune=native)

target_include_directories(unit-test PRIVATE 
  src
  kc1fsz-tools-cpp/include
  kc1fsz-tools-cpp/include/kc1fsz-tools/crc
  itu-g711-codec/src
  cmsis-dsp-mock/include
  cobs-c
  amp-core/include
  amp-core/src
  sound-map/include
  ed25519/src
  kc1fsz-sdrc/sw/src
  kc1fsz-sdrc/sw/include
  cpp-httplib
  base64.c
  json/include
  argparse/include
  g726-codec/src
  )
target_link_libraries(unit-test -lasound -lcurl -lusb-1.0)

# ----- DECTalk Library -----

set(NO_FILESYSTEM OFF)
set(NO_CHARSET OFF)
option(DECTALKMINI_NO_FILESYSTEM "Disable filesystem" ${NO_FILESYSTEM})
option(DECTALKMINI_NO_CHARSET "Disable charset conversion" ${NO_CHARSET})
option(DECTALKMINI_EXAMPLES "Build examples" ON)

file(GLOB DTC_SOURCES DECtalkMini/src/*.c)

add_library(dtc STATIC ${DTC_SOURCES})

target_include_directories(dtc PUBLIC 
  DECtalkMini/include
)
target_compile_definitions(dtc PRIVATE
	"_REENTRANT"
	"NOMME"
	"LTSSIM"
	"TTSSIM"
	"ANSI"
	"BLD_DECTALK_DLL"
	"ENGLISH"
	"ENGLISH_US"
	"ACCESS32"
	"TYPING_MODE"
	"ACNA"
	"DISABLE_AUDIO"
	"SINGLE_THREADED"
)
target_compile_definitions(dtc PUBLIC
	"DT_EXTERN="
)

add_executable(dec-talk-1 EXCLUDE_FROM_ALL
  DECtalkMini/main.c 
)
# NOTE: Compiler optimizations are turned on
target_compile_options(dec-talk-1 PRIVATE -Wall -Wpedantic -g -O3 -mtune=native)
target_include_directories(dec-talk-1 PRIVATE 
  DECtalkMini/include 
)
target_link_libraries(dec-talk-1 PRIVATE 
  dtc
  m
)
