cmake_minimum_required(VERSION 3.15)
# set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum OS X deployment target")
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 7.1 CACHE STRING INTERNAL FORCE) # Windows SDK for Windows 7 and up
endif()
project(HHOOMP VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")

# Build universal binary for Intel and Apple Silicon compatibility
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")

# Fix C++ standard library path for Xcode
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/v1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    # Disable precompiled headers for now
    set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
endif()
set(COMPANY_NAME "Headroom Heros")
set(PRODUCT_NAME "HHOOMPH")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/modules/cmake")
add_subdirectory(modules)

# set default plugin formats to build
if(IOS)
    set(JUCE_FORMATS Standalone AUv3)
else()
    set(JUCE_FORMATS AU VST3 Standalone)
endif()

juce_add_plugin(HHOOMP
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "com.headroomheros.hhoomph"
    PLUGIN_MANUFACTURER_CODE Hdrm
    PLUGIN_CODE HMPH
    FORMATS ${JUCE_FORMATS}
    PRODUCT_NAME "${PRODUCT_NAME}"
    
    LV2URI https://github.com/Chowdhury-DSP/HHOOMP

    MICROPHONE_PERMISSION_ENABLED FALSE
)

# CLAP support disabled due to JUCE compatibility issues
# if(NOT IOS)
#     clap_juce_extensions_plugin(
#         TARGET HHOOMP
#         CLAP_ID "org.headroomheros.HHOOMP"
#         CLAP_FEATURES audio-effect
#         CLAP_PROCESS_EVENTS_RESOLUTION_SAMPLES 64
#         CLAP_USE_JUCE_PARAMETER_RANGES DISCRETE
#     )
# endif()

add_subdirectory(src)
include_directories(src)
add_subdirectory(res)

# Add designer assets to BinaryData
juce_add_binary_data(Assets SOURCES
    assets/interface_dsn_VST_wht-bckgnd-0202.png
    assets/dial_knob.png
    assets/switch-toggle-off.png
    assets/switch-toggle-on.png
)
target_sources(HHOOMP PRIVATE $<TARGET_OBJECTS:Assets>)
target_include_directories(HHOOMP PRIVATE ${CMAKE_BINARY_DIR}/juce_binarydata_Assets/JuceLibraryCode)

target_compile_definitions(HHOOMP PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_JACK=0
    JUCE_JACK=0
)

target_link_libraries(HHOOMP PRIVATE juce_plugin_modules)
