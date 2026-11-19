# cmake -S . -B build -A x64 -DSMS_MODS_OUT=C:/path/to/the/game/mods   (Windows 64-bit)
# cmake --build build --config Release
cmake_minimum_required(VERSION 3.16)
project(christmas_additions CXX)

# The SDK shipped here (sdk/): Super Mario Sunshine Remastered's, for api->parts (joints and a
# part's colour); everything else is Chase's SDK as it is.
set(FETCHCONTENT_SOURCE_DIR_SMS_MOD_SDK ${CMAKE_CURRENT_SOURCE_DIR}/sdk)
include(FetchContent)
FetchContent_Declare(sms_mod_sdk
  GIT_REPOSITORY https://github.com/Allebale/sms-mod-sdk-modtesting.git
  GIT_TAG parts)
FetchContent_MakeAvailable(sms_mod_sdk)

sms_add_mod(mod src/christmas_additions.cpp)
set_target_properties(${SMS_MOD_TARGET} PROPERTIES CXX_STANDARD 11)
if(MSVC)
  target_compile_options(${SMS_MOD_TARGET} PRIVATE /W3 /D_CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(${SMS_MOD_TARGET} PRIVATE -Wall -Wextra -O2)
endif()
