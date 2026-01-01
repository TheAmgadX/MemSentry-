cmake_minimum_required(VERSION 3.13)
project(MemSentry VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ==========================================
# 1. Output Configuration
# ==========================================
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ==========================================
# 2. Include Paths
# ==========================================
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# ==========================================
# 3. Build the Library (MemSentry)
# ==========================================
add_library(
    MemSentry STATIC
    src/mem_sentry.cc
)

# ==========================================
# 4. Build the Example App (Client)
# ==========================================
add_executable(application
    examples/main.cc
)

target_link_libraries(
    application
    PRIVATE MemSentry
)