# Concurrent-Matrix-Operations

<img width="1000" height="700" alt="matrix_benchmark" src="https://github.com/user-attachments/assets/a8bddf81-4c3b-4054-8868-b879f997ff6f" />

A high-performance C++ pipeline implementing three core matrix operations: transposition, zone summation, and self-multiplication. Each operation has been parallelised to accelerate execution. The three operations are defined as follows:

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
Each cell in the destination matrix contains the sum of the corresponding cell in the source matrix and its surrounding neighbours. Only cells within the matrix boundaries are included: corner cells have three neighbours, non-corner edge cells have five, and interior cells have eight. The destination matrix has the same dimensions as the source matrix.

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
The matrix produced by the zone-sum stage is multiplied by itself. Matrix multiplication combines two matrices by calculating the sum of the products of corresponding elements from the rows of the first matrix and the columns of the second. 

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

The following table outlines the average execution time (calculated across 10 consecutive runs) for processing the full pipeline for a 1024 x 1024 matrix. 

| Implementation | Execution Time (ms) | Speed-up |
| :--- | :--- | :--- |
| **Sequential (Baseline)** | 274.767 ms | 1.00x |
| **Parallel Version**| 52.132 ms | 5.27x |

*Hardware Context: Benchmarks were recorded on an AMD Ryzen 5, 6 Cores / 12 Threads, Windows 11 system.*

### Trade-off Analysis
*   **Thread Management Overhead:** Creating threads introduces an initial cost. This implementation uses a thread pool: a group of worker threads created once and reused across operations. Assigning work to these threads and waiting for them to finish each stage still adds overhead, which can outweigh the benefits of parallel execution for small matrices. For larger matrices, the benefits of parallel processing are more likely to outweigh this overhead.
*   **Memory Access Patterns & Contiguity:** The source matrix is copied from a nested `std::vector` into a contiguous 1D array before processing. The zone-sum stage also produces a transposed copy of its result, allowing the multiplication stage to read both input operands sequentially within each dot product.
*   **Cache Blocking (Tiling):** The multiplication algorithm divides the work into blocks of up to 64 × 64 elements, accumulating each output block over smaller sections of the dot products. This is intended to improve cache locality by reusing matrix values within each block before moving on.

---

## Installation & Setup

#### Prerequisites
* A modern C++ compiler (MSVC for Windows, `g++` for Linux)
* CMake (Version 3.15+)
* Git

#### Option A: Cross-Platform Build (Recommended)
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

#### Option B: Direct G++ Compilation (Linux)
If you prefer to bypass CMake on a Linux environment, you can compile directly using the GNU compiler:
```bash
g++ *.cpp -o matrix_operations -pthread -O3 -march=native
./matrix_operations
```

**Compiler flags explained:**
   * **`-O3` (Level 3 Optimisation):** Instructs the `g++` compiler to apply available optimisation techniques. These include loop unrolling, vectorisation, and function inlining to prioritise maximum execution speed over compilation time and binary size.
   * **`-march=native` (Architecture Native):** Instructs the compiler to tune the generated machine code specifically for the CPU architecture of the machine currently compiling it. This allows the executable to utilise hardware-specific instruction sets for maximum throughput, though it makes the resulting binary non-portable to different CPU architectures.

---

## Acknowledgments
This project utilizes a C++ ThreadPool library originally created by Jakob Progsch and Václav Zeman.
