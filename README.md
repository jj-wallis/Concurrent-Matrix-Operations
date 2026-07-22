# Concurrent-Matrix-Operations

<img width="1000" height="700" alt="matrix_benchmark" src="https://github.com/user-attachments/assets/a8bddf81-4c3b-4054-8868-b879f997ff6f" />

A high performance C++ application designed to three core matrix operations: transposition, zone summation and a self multiplication. These three operations are executed sequentially upon an input matrix; however each operation has been parallelised to accelerate execution. The aim of parallel computation is to solve intensive problems with a reduced computation time and efficiently handle large-scale data processing tasks. The three operations have been defined as follows: 

### 1. Matrix Transposition
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

---

### 2. Zone Sum
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

---

### 3. Matrix Multiplication
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


## Performance Benchmarks

The following table outlines the average execution time (calculated across 10 consecutive runs) for processing a 1024 x 1024 matrix. 

| Implementation | Execution Time (ms) | Speed-up |
| :--- | :--- | :--- |
| **Sequential (Baseline)** | 274.767 ms | 1.00x |
| **Parallel Version**| 52.132 ms | 5.27x |

*Hardware Context: Benchmarks were recorded on an AMD Ryzen 5, 6 Cores / 12 Threads system.*

### Trade-off Analysis
*   **Thread Spawning Overhead:** While parallel execution provides massive speed benefits for large datasets, managing threads introduces an OS-level performance cost. Spawning and destroying a thread requires system calls and context switching. For very small matrices, this overhead can make the multithreaded version slower than a sequential solution. However, as matrix dimensions scale, the computational workload dwarfs thread creation cost, resulting in increased performance for multithreaded architectures.
*   **Memory Access Patterns & Contiguity::** Standard matrix multiplication using 2D std::vector structures forces the CPU to chase pointers across fragmented memory, incurring cache misses. To resolve this, the algorithm first flattens 2D structures into contiguous 1D arrays. Furthermore, the second matrix (used for self multiplication) is explicitly transposed during the zone sum phase, ensuring that the subsequent dot-products can be calculated using strictly sequential, hardware-friendly row-major reads.
*   L1 Cache Tiling: Even with contiguous and transposed memory, processing entire rows of massive matrices linearly can quickly overflow the CPU's cache boundaries. To prevent this, the core multiplication algorithm utilises a 64x64 blocked tiling technique. By subdividing the workload into localised blocks, the algorithm restricts active working data to fit entirely within L1 cache.

## Build Instructions

## Acknowledgments
This project utilizes a C++ ThreadPool library originally created by Jakob Progsch and Václav Zeman.
