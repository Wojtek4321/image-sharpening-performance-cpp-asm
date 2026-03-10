# Image Sharpening Performance: C++ vs x64 Assembly 🚀

A high-performance engineering project comparing the efficiency of image processing algorithms. This tool implements an image sharpening filter in both high-level **C++** and low-level **x64 Assembly** to demonstrate hardware-level optimization.

## 🛠️ Tech Stack & Optimization
* **Languages:** C++, x64 Assembly (Intel syntax)
* **Hardware Acceleration:** SIMD Instructions (SSE/AVX) for vectorization
* **Parallel Computing:** Multi-threading for split-image processing
* **Tools:** MSVC Compiler / Visual Studio

## 💡 Key Features
* **Manual Memory Management:** Precise control over pixel data buffers.
* **SIMD Implementation:** Processing multiple pixels in a single CPU instruction cycle using AVX/SSE registers.
* **Benchmarking:** High-precision timers to compare execution speed between C++ and ASM implementations.

## 🚀 How to Run
1. Clone the repository.
2. Open the project in **Visual Studio**.
3. Ensure the build configuration is set to `x64`.
4. Run the application and provide a `.bmp` or `.png` file as input.
