cmake_minimum_required(VERSION 3.15)
project(MemSentryExamples VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find the MemSentry library (assumes it's installed or built in the parent project)
find_package(MemSentry REQUIRED)

# Toggle Sentry on/off.
# Disable in release builds and performance-critical or stress tests due to runtime overhead.
# Enable only for debugging and development environments.
set(MEM_SENTRY_ENABLE ON CACHE BOOL "" FORCE) 
# set(MEM_SENTRY_ENABLE OFF CACHE BOOL "" FORCE) turn off.

# Example 1: Allocation/Deallocation Reporting
add_executable(example_allocation_reporting example_allocation_reporting.cc)
target_link_libraries(example_allocation_reporting PRIVATE MemSentry::MemSentry)

# Example 2: Enable/Disable
add_executable(example_enable_disable example_enable_disable.cc)
target_link_libraries(example_enable_disable PRIVATE MemSentry::MemSentry)

# Example 3: MemoryReport from Heap
add_executable(example_memory_report example_memory_report.cc)
target_link_libraries(example_memory_report PRIVATE MemSentry::MemSentry)

# Example 4: Hierarchical Graph Reporting
add_executable(example_hierarchical_graph example_hierarchical_graph.cc)
target_link_libraries(example_hierarchical_graph PRIVATE MemSentry::MemSentry)

# Example 5: Memory Pools Usage
add_executable(example_memory_pools example_memory_pools.cc)
target_link_libraries(example_memory_pools PRIVATE MemSentry::MemSentry)

# Example 6: Strategy Pattern/Interfaces
add_executable(example_strategy_pattern example_strategy_pattern.cc)
target_link_libraries(example_strategy_pattern PRIVATE MemSentry::MemSentry)
