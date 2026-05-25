cmake_minimum_required(VERSION 3.22.1)
project("sanki-core")

# C++ core sources
set(CORE_SOURCES
    deck_loader.cpp
    sm2_scheduler.cpp
    session_manager.cpp
    sanki_core.cpp
)

# C sources (SQLite)
set(C_SOURCES
    sqlite3/sqlite3.c
)

# JNI bridge
set(JNI_SOURCES
    jni_bridge.cpp
)

# Build the shared library
add_library(sanki-core SHARED
    ${CORE_SOURCES}
    ${C_SOURCES}
    ${JNI_SOURCES}
)

# Include directories
target_include_directories(sanki-core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3
)

# Compile options for C++
target_compile_options(sanki-core PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=c++20>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall>
    $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
    $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-frtti>
    -O2
)

# SQLite needs thread safety
target_compile_definitions(sanki-core PRIVATE
    SQLITE_THREADSAFE=1
    SQLITE_OMIT_LOAD_EXTENSION
    SQLITE_ENABLE_FTS5
)

# Link libraries
find_library(log-lib log)

target_link_libraries(sanki-core
    ${log-lib}
    z
)
