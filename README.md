# Image Sharpening Performance: C++ vs x64 Assembly

Low-level implementation and performance analysis of an image sharpening filter. The project compares a  C++ approach with manual x64 Assembly optimization using SIMD instructions.

## Technical Overview
* **Optimization:** x64 Assembly (Intel syntax), SIMD (SSE/AVX vectorization).
* **Parallelism:** Multi-threaded execution for workload distribution.
* **Environment:** MSVC / Visual Studio 2022.
* **Focus:** Hardware-level data processing and memory alignment.
* 
Algorithm DetailThe sharpening process is based on the following logic:
Box Blur (3x3): Calculating a blurred version of the source image.
Sharpening Formula: dst = src + amount*(src - blur).
Intensity Adjustment: Adjustable via GUI slider (0-200%), mapped to a factor of 0.0 - 2.0

## Benchmarks
**Test Data:** 1200x678 (105 KB) | **Intensity:** 200%

| Threads | ASM (ms) | C++ (ms) | Speedup |
| :--- | :--- | :--- | :--- |
| 1 | 12.348 | 15.030 | 1.22x |
| 2 | **10.366** | **12.122** | 1.17x |
| 4 | 10.840 | 12.668 | 1.17x |
| 8 | 12.688 | 15.164 | 1.19x |
| 16 | 20.216 | 25.950 | 1.28x |
| 32 | 25.574 | 36.432 | 1.42x |
| 64 | 35.978 | 58.970 | 1.64x |

### Execution Time Comparison (ms)
| Image Size | Library | 1 Thread | 2 Threads | 8 Threads | 64 Threads |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **S (1200x678)** | **ASM** | 12.348 | **10.366** | 12.688 | 35.978 |
| | C++ | 15.030 | 12.122 | 15.164 | 58.970 |
| **M (2912x4012)** | **ASM** | 167.494 | **133.371** | 139.118 | 221.204 |
| | C++ | 196.146 | 144.842 | 141.994 | 247.188 |
| **L (4032x3024)** | **ASM** | 177.058 | 143.322 | **137.972** | 238.836 |
| | C++ | 205.300 | 153.016 | 139.150 | 247.526 |

### Performance Analysis
* **Optimal Concurrency:** Peak performance was achieved with **2 threads**. 
* **Threading Overhead:** Increasing threads beyond the physical core limit (or for small data sets) resulted in performance degradation due to context switching and synchronization costs.
* **ASM vs C++:** The x64 Assembly implementation consistently outperformed C++ by utilizing efficient register management and vector instructions, showing up to **64%** gain under high load.

## Setup & Execution
1. Clone the repository.
2. Open the solution in **Visual Studio 2022**.
3. Set Build Configuration to **Release | x64**.
4. Run the executable with the source image path as an argument.
