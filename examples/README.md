# 📦 Installation

You can install MemSentry using the pre-packaged release or by building from source.

### Option A: Install via Package
Download the latest `.deb` file from the [Releases Page](https://github.com/TheAmgadX/MemSentry/releases). This will install the headers and CMake configuration files to your system automatically.

```bash
# Replace with the actual version you downloaded
sudo dpkg -i MemSentryLibrary-1.0.0-Linux.deb
```

### Option B: Using the .zip Release (Manual Integration)

If you downloaded the `.zip` release, you have the headers, source files, and CMake configuration, but not the full project build system. To use it in your project:

1.  **Unzip** the file into a directory (e.g., `~/libs/MemSentry/`).
2.  **Point your project to the library**: In your project's `CMakeLists.txt`, set the `MemSentry_DIR` to the path containing the `.cmake` files before calling `find_package`.

#### CMake Configuration
```cmake
cmake_minimum_required(VERSION 3.15)
project(MyProject)

# 1. Tell CMake where the .zip contents are located
# This should point to the folder containing MemSentryConfig.cmake
set(MemSentry_DIR "$ENV{HOME}/libs/MemSentry/lib/cmake/MemSentry" CACHE PATH "")

# 2. Find the package
find_package(MemSentry REQUIRED)

add_executable(MyProject main.cc)

# 3. Link the Interface Library
target_link_libraries(MyProject PRIVATE MemSentry::MemSentry)
```

## 🚀 Running the Examples
If you built the project with `MEM_SENTRY_BUILD_EXAMPLES=ON` (default), the example executables are located in the `build/` directory.

| Example Executable | Description |
| :--- | :--- |
| `./example_allocation_reporting` | Demonstrates basic allocation tracking and attaching a reporter. |
| `./example_enable_disable` | Shows how to compile MemSentry out of the build completely. |
| `./example_memory_report` | Shows how to snapshot memory usage between two points in time. |
| `./example_hierarchical_graph` | Connects multiple Heaps (Graph Representation) for total usage tracking. |
| `./example_memory_pools` | Uses Fixed-Size Pools and Pool Chain. |
| `./example_strategy_pattern` | **(Important)** Demonstrates plugging in custom Reporters (e.g., logging to File/Console) and usage of ISentry Interface |

**To run an example:**
```bash
cd build
./example_allocation_reporting
```

# 🛠 Integration Guide (Critical!)
To use MemSentry in your own C++ application, you must follow these linking and coding steps strictly.

### Step A: CMake Configuration (CMakeLists.txt)
Find the library using CMake and link it to your executable.

```cmake
cmake_minimum_required(VERSION 3.15)
project(MyGame)

# 1. Find the installed MemSentry package
find_package(MemSentry REQUIRED)

add_executable(MyGame main.cc)

# 2. Link the library
target_link_libraries(MyGame PRIVATE MemSentry::MemSentry)

# Toggle Sentry on/off.
# Disable in release builds and performance-critical or stress tests due to runtime overhead.
# Enable only for debugging and development environments.
set(MEM_SENTRY_ENABLE ON CACHE BOOL "" FORCE) 
# set(MEM_SENTRY_ENABLE OFF CACHE BOOL "" FORCE) turn off.
```
#### ⚠️ IMPORTANT: 
- You must include mem_sentry/mem_sentry.h in any file where you allocate memory using a Heap. If you forget this include, the compiler will use the standard C++ new (no header), but the linker may use MemSentry's delete (expects header), causing a CRASH.

## 📝 License
This project is licensed under the MIT License - **[LICENSE](../LICENSE)**