# Concurrent-Matrix-Operations

<img width="1000" height="700" alt="matrix_benchmark" src="https://github.com/user-attachments/assets/a8bddf81-4c3b-4054-8868-b879f997ff6f" />

A high-performance C++ application implementing three core matrix operations: transposition, zone summation, and self-multiplication. These three operations are executed sequentially on an input matrix, with each operation parallelised to accelerate execution. The aim of parallel computation is to solve intensive problems in reduced time and efficiently handle large-scale data processing tasks. The three operations are defined as follows:

## 1. Matrix Transposition
Matrix transposition reflects a matrix across its main diagonal, converting an m x n matrix into an n x m matrix by swapping its rows with its columns.

**Example:**

*Source*
| 1 | 2 | 3 | 4 |
| :---: | :---: | :---: | :---: |
| 5 | 6 | 7 | 8 |
| 9 | 10 | 11 | 12 |
| 13 | 14 | 15 | 16 |

*Destination*
| 1 | 5 | 9 | 13 |
| :---: | :---: | :---: | :---: |
| 2 | 6 | 10 | 14 |
| 3 | 7 | 11 | 15 |
| 4 | 8 | 12 | 16 |

## 2. Zone Sum
The value of a single cell in the source matrix is summed with its neighbouring cells in the destination
matrix. Corner values in the destination matrix result from the sum of the corner value and its three
neighbours from the source matrix. Values along the sides of the destination matrix are the result of the
sum of the corresponding value in the source matrix with its five neighbouring values. Consequently,
central values result from the sum of the corresponding value in the source matrix with its 8 neighbours.
The destination matrix retains the same size as the source matrix.

**Example:**

*Source*
| 0 | 0 | 0 | 0 |
| :---: | :---: | :---: | :---: |
| 0 | 99 | 0 | 0 |
| 0 | 0 | 0 | 0 |
| 0 | 0 | 0 | 50 |

*Destination*
| 99 | 99 | 99 | 0 |
| :---: | :---: | :---: | :---: |
| 99 | 99 | 99 | 0 |
| 99 | 99 | 149 | 50 |
| 0 | 0 | 50 | 50 |

## 3. Matrix Multiplication
Matrix multiplication combines two matrices by calculating the sum of the products of corresponding elements from the rows of the first matrix and the columns of the second. 

**Example:**

*Source Matrix 1*
| a1 | a2 |
| :---: | :---: |
| a3 | a4 |

*Source Matrix 2*
| b1 | b2 |
| :---: | :---: |
| b3 | b4 |

*Resulting Matrix*
| (a1 * b1) + (a2 * b3) | (a1 * b2) + (a2 * b4) |
| :---: | :---: |
| (a3 * b1) + (a4 * b3) | (a3 * b2) + (a4 * b4) |

---

## Performance Benchmarks

The following table outlines the average execution time (calculated across 10 consecutive runs) for processing a 1024 x 1024 matrix. 

| Implementation | Execution Time (ms) | Speed-up |
| :--- | :--- | :--- |
| **Sequential (Baseline)** | 274.767 ms | 1.00x |
| **Parallel Version**| 52.132 ms | 5.27x |

*Hardware Context: Benchmarks were recorded on an AMD Ryzen 5, 6 Cores / 12 Threads, Windows 11 system.*

### Trade-off Analysis
*   **Thread Spawning Overhead:** While parallel execution provides massive speed benefits for large datasets, managing threads introduces an OS-level performance cost. Spawning and destroying a thread requires system calls and context switching. For very small matrices, this overhead can make the multithreaded version slower than a sequential solution. However, as matrix dimensions scale, the computational workload dwarfs thread creation cost, resulting in increased performance for multithreaded architectures.
*   **Memory Access Patterns & Contiguity:** Standard matrix multiplication using 2D std::vector structures forces the CPU to chase pointers across fragmented memory, incurring cache misses. To resolve this, the algorithm first flattens 2D structures into contiguous 1D arrays. Furthermore, the second matrix (used for self multiplication) is explicitly transposed during the zone sum phase, ensuring that the subsequent dot-products can be calculated using strictly sequential, hardware-friendly row-major reads.
*   **L1 Cache Tiling:** Even with contiguous and transposed memory, processing entire rows of massive matrices linearly can quickly overflow the CPU's cache boundaries. To prevent this, the core multiplication algorithm utilises a 64x64 blocked tiling technique. By subdividing the workload into localised blocks, the algorithm restricts active working data to fit entirely within L1 cache.

---

## Installation & Setup

#### Prerequisites
* A modern C++ compiler (MSVC for Windows, `g++` for Linux)
* CMake (Version 3.15+)
* Git

#### Option A: Cross-Platform Build (Recommended - CMake)
Works identically across Windows, Linux, and macOS. It will automatically apply hardware-specific optimisations (`-O3 -march=native`) where applicable.

1. **Clone the repository:**
   ```bash
   git clone https://github.com/jj-wallis/Concurrent-Matrix-Operations.git
   cd Concurrent-Matrix-Operations
   ```
2. **Configure the build:**
   ```bash
   cmake -B build -DCMAKE_BUILD_TYPE=Release
   ```
3. **Compile the binary:**
   ```bash
   cmake --build build --config Release
   ```
4. **Run the executable:**
   * **Linux/macOS:** `./build/matrix_operations`
   * **Windows:** `.\build\Release\matrix_operations.exe`

#### Option B: Direct GCC Compilation (Linux Only)
If you prefer to bypass CMake on a Linux environment, you can compile directly using the GNU compiler:
```bash
g++ *.cpp -o matrix_operations -pthread -O3 -march=native
./matrix_operations
```

**Compiler flags explained:**
   * **`-O3` (Level 3 Optimisation):** Instructs the `g++` compiler to aggressively apply available optimisation techniques. These include loop unrolling, vectorisation, and function inlining to prioritise maximum execution speed over compilation time and binary size.
   * **`-march=native` (Architecture Native):** Instructs the compiler to tune the generated machine code specifically for the CPU architecture of the machine currently compiling it. This allows the executable to utilise hardware-specific instruction sets for maximum throughput, though it makes the resulting binary non-portable to different CPU architectures.

#### Option C: Visual Studio (Windows Only)
Modern Visual Studio natively supports CMake. 
1. Open Visual Studio and select `File > Open > Folder...`
2. Select the cloned `Concurrent-Matrix-Operations` folder.
3. Visual Studio will automatically detect the `CMakeLists.txt` file. 
4. Select `Release` from the configuration dropdown in the toolbar and click the green `Play` button to build and run.

---

## Acknowledgments
This project utilizes a C++ ThreadPool library originally created by Jakob Progsch and Václav Zeman.
