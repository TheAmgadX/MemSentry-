# ==============================================================================
# SETUP AND CONFIGURATION
# ==============================================================================
cmake_minimum_required(VERSION 3.15)

project(MemSentryLibrary VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------------------
# Options
# These allow the User to toggle features on/off in THEIR build.
# ------------------------------------------------------------------------------
option(MEM_SENTRY_ENABLE "Enable memory tracking features" ON)
option(MEM_SENTRY_BUILD_EXAMPLES "Build example applications" ON)
option(MEM_SENTRY_BUILD_TESTS "Build unit tests" ON)

# ==============================================================================
# DEFINE THE LIBRARY
# ==============================================================================
add_library(MemSentry INTERFACE)

# ------------------------------------------------------------------------------
# Attach the Source Code (.cc files)
# ------------------------------------------------------------------------------
target_sources(MemSentry INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/mem_sentry.cc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/heap.cc>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/console_reporter.cc>
    
    # assume will install the 'src' folder to the installation root.
    $<INSTALL_INTERFACE:src/mem_sentry.cc>
    $<INSTALL_INTERFACE:src/heap.cc>
    $<INSTALL_INTERFACE:src/console_reporter.cc>
)

# ------------------------------------------------------------------------------
# Attach the Headers (.h files)
# ------------------------------------------------------------------------------
target_include_directories(MemSentry INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ------------------------------------------------------------------------------
# Apply the Configuration Flags
# ------------------------------------------------------------------------------
# If the user turns this ON, pass "MEM_SENTRY_ENABLE=1" to the compiler.
# Use INTERFACE so this flag propagates to the User's Application.
if(MEM_SENTRY_ENABLE)
    target_compile_definitions(MemSentry INTERFACE MEM_SENTRY_ENABLE=1)
else()
    target_compile_definitions(MemSentry INTERFACE MEM_SENTRY_ENABLE=0)
endif()

# ==============================================================================
# INSTALLATION RULES
# ==============================================================================
include(GNUInstallDirs)

# ------------------------------------------------------------------------------
# Rule 1: Install the Source Code
# ------------------------------------------------------------------------------
install(DIRECTORY src/
    DESTINATION src 
)

# ------------------------------------------------------------------------------
# Rule 2: Install the Headers
# ------------------------------------------------------------------------------
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# ------------------------------------------------------------------------------
# Rule 3: Install the Library Target Definition
# ------------------------------------------------------------------------------
install(TARGETS MemSentry
    EXPORT MemSentryTargets
)

# ==============================================================================
# EXPORT CONFIGURATION
# ==============================================================================
# This creates the "MemSentryTargets.cmake" file.
# When a user runs find_package(MemSentry), this file is loaded.
install(EXPORT MemSentryTargets
    FILE MemSentryTargets.cmake
    NAMESPACE MemSentry::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MemSentry
)

# Create version file (so users can check if they have v1.0.0 or v2.0.0)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "MemSentryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the config version file
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MemSentryConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MemSentry
)

# ==============================================================================
# SUBDIRECTORIES (Tests & Examples)
# ==============================================================================
if(MEM_SENTRY_BUILD_EXAMPLES)
    add_executable(application examples/main.cc)
    target_link_libraries(application PRIVATE MemSentry)
endif()

if(MEM_SENTRY_BUILD_TESTS)
    add_subdirectory(tests)
endif()


# ==============================================================================
# PACKAGING (CPack)
# ==============================================================================
set(CPACK_PACKAGE_VENDOR "TheAmgadX")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A memory management library for C++")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_CONTACT "theamgadx@gmail.com")

set(CPACK_GENERATOR "DEB;ZIP") 

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "TheAmgadX")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)