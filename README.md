# MemSentry

**MemSentry** is a high-performance C++20 memory management library designed to provide transparency and control over your application's memory usage. It features advanced tracking, leak detection, custom heap hierarchies, and lock-free memory pools.

---

## ✨ Key Features

* **Global Tracking**: Overrides global `new` and `delete` operators to automatically track all memory allocations.
* **Custom Heaps**: Organize memory into named Heaps (e.g., "Physics", "AI") and connect them in a hierarchy.
* **Leak Detection**: Snapshot memory states and detect leaks between specific execution points.
* **Lock-Free Pools**: High-performance *SPSC* (Single-Producer Single-Consumer) ***Ring Pool*** and dynamically growing pools ***Chain Pool*** designed for low-latency applications.
* **Strategy Pattern Reporting**: Inject custom reporters (Console, File, Network) to handle memory events.
* **Seamless Integration**: The `ISentry` interface allows you to easily hook your custom classes into the tracking system with minimal boilerplate.
* **Production Ready**: Easily compile the entire library out of your build for release using `MEM_SENTRY_ENABLE`.

---

## 📦 Installation

MemSentry is an **Interface Library** that compiles directly into your project.

> For detailed installation options, including `.deb` package generation, see the ***[Installation Guide](./examples/README.md#%F0%9F%93%A6-1-installation)***.

---

## 📚 Documentation & Usage

We provide a comprehensive set of examples and an integration guide to help you get started.

👉 **[Read the Integration Guide & Examples](./examples/README.md)**

Inside the **[`examples/`](./examples/)** folder, you will learn:

* **How to Link**: CMake configuration for your own projects.
* **How to Code**: Critical rules for including headers to avoid crashes.
* **Demos**:
    * `example_allocation_reporting.cc`: Basic tracking.
    * `example_memory_pools.cc`: Using lock-free pools.
    * `example_hierarchical_graph.cc`: Managing heap hierarchies.

#### See ***[Documentation](./docs)***

---

## 📂 Project Structure

* **`include/mem_sentry/`**: Core memory tracking headers (Heap, Sentry, Allocator).
* **`include/mem_pools/`**: Lock-free pool and buffer headers.
* **`src/`**: Library source code (compiled into your target).
* **`examples/`**: Reference implementations and the **Main User Guide**.
* **`tests/`**: Unit tests for library stability.
* **`docs/`**: Documentation and Class Diagrams.

---

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.