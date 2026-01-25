cmake_minimum_required(VERSION 3.15)
project(z8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optimization flags
if(MSVC)
    add_compile_options(/O2 /MT /Oi /Ot /EHsc /Zc:__cplusplus /nologo)
    add_definitions(-DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX -DNDEBUG)
else()
    add_compile_options(-O3 -pthread)
    add_definitions(-DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX -DNDEBUG)
endif()

include_directories(include src)

set(SOURCES
    src/main.cpp
    src/temporal_shims.cpp
    src/module/console.cpp
    src/module/node/fs/fs.cpp
    src/module/timer.cpp
)

add_executable(z8 ${SOURCES})

# Linking
if(WIN32)
    target_link_libraries(z8 PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/v8_monolith.lib
        winmm dbghelp shlwapi user32 iphlpapi advapi32 shell32 ole32 uuid rpcrt4 ntdll
    )
elseif(APPLE)
    # Placeholder for macOS V8 lib
    target_link_libraries(z8 PRIVATE
        v8_monolith
    )
else()
    # Placeholder for Linux V8 lib
    target_link_libraries(z8 PRIVATE
        v8_monolith
        pthread
        dl
    )
endif()
