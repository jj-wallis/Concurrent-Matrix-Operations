#include <vector>
#include <thread>
#include <memory>
#include <algorithm>

#include "MatrixOperations.h"
#include "FileWrite.h"
#include "ThreadPool.h"

#define PARALLEL_MODE 1

// =================================================================================
// Forward Declarations
// =================================================================================
void flattenWorker(const std::vector<std::vector<double>>* srcMatrix, double* flatSrc, const int startRow, const int endRow, const int dim, barrier* syncBarrier);
void zoneSumTransposeWorker(const double* flatSrc, double* flatZS, double* flatZS_transposed, const int startRow, const int endRow, const int dim, barrier* syncBarrier);
void matrixMultiplyWorker(const double* flatZS, const double* flatZS_transposed, std::vector<std::vector<double>>* dstMatrix, const int startRow, const int endRow, const int dim, barrier* syncBarrier);

// =================================================================================
// Distributed Matrix Operations Pipeline
// =================================================================================
void matrixOperationsInit(std::vector<std::vector<double>>* srcMatrix, std::vector<std::vector<double>>* dstMatrix)
{
    // Get dimensions of the src matrix
    const int dim = srcMatrix->size();

    // Allocate memory for flattened arrays on the heap to allow for large matrices
    // Allocate memory for flattened arrays using smart pointers to guarantee exception safety
    std::unique_ptr<double[]> flatSrc = std::make_unique<double[]>(dim * dim); // Source matrix
    std::unique_ptr<double[]> flatZS = std::make_unique<double[]>(dim * dim); // Result of zone sum
    std::unique_ptr<double[]> flatZS_transposed = std::make_unique<double[]>(dim * dim); // Result of zone sum and transpose

#if PARALLEL_MODE

    // Spawn a number of threads equal to the machine's number of cores
    // Static ensures these are created once during the first function call
    static const int numThreads = std::thread::hardware_concurrency();
    // Instantiate a threadpool using the value from hardware_concurrency()
    static ThreadPool threadPool(numThreads);

    // Initialise barrier for all threads
    static barrier syncBarrier(numThreads + 1); // +1 accounts for the main thread

    // Calculate the workload distribution
    const int rowsPerThread = dim / numThreads;
    const int extraRows = dim % numThreads;

    // ===================================================
    // Flatten 2D vector to 1D array
    // (Write to flatSrc)
    // ===================================================
    int currentRow = 0;
    for (int t = 0; t < numThreads; t++) // Dispatch tasks to thread pool
    {
        const int startRow = currentRow;
        const int endRow = startRow + rowsPerThread + (t < extraRows ? 1 : 0); // Add an extra row to the first t threads if dim did not divide evenly
        threadPool.enqueue(flattenWorker, srcMatrix, flatSrc.get(), startRow, endRow, dim, &syncBarrier); // Spawn a worker thread
        currentRow = endRow;
    }
    // Block main thread until all worker threads have completed. Barrier resets automatically
    syncBarrier.count_down_and_wait(); 

    // ===================================================
    // Fused Zone Sum and Transpose 
    // (Dual Write to flatZS and flatZS_transposed)
    // ===================================================
    currentRow = 0;
    for (int t = 0; t < numThreads; t++)
    {
        const int startRow = currentRow;
        const int endRow = startRow + rowsPerThread + (t < extraRows ? 1 : 0);
        threadPool.enqueue(zoneSumTransposeWorker, flatSrc.get(), flatZS.get(), flatZS_transposed.get(), startRow, endRow, dim, &syncBarrier);
        currentRow = endRow;
    }
    // Block main thread
    syncBarrier.count_down_and_wait();

    // ===================================================
    // Blocked Matrix Multiplication 
    // (Direct Write to dstMatrix)
    // ===================================================
    currentRow = 0;
    for (int t = 0; t < numThreads; t++)
    {
        const int startRow = currentRow;
        const int endRow = startRow + rowsPerThread + (t < extraRows ? 1 : 0);
        threadPool.enqueue(matrixMultiplyWorker, flatZS.get(), flatZS_transposed.get(), dstMatrix, startRow, endRow, dim, &syncBarrier);
        currentRow = endRow;
    }
    // Block main thread
    syncBarrier.count_down_and_wait();

#else // Sequential execution

    // ===================================================
    // 1. Flatten 2D vector to 1D array
    // ===================================================
    flattenWorker(srcMatrix, flatSrc.get(), 0, dim, dim, nullptr);

    // ===================================================
    // 2. Fused Zone Sum and Transpose 
    // ===================================================
    zoneSumTransposeWorker(flatSrc.get(), flatZS.get(), flatZS_transposed.get(), 0, dim, dim, nullptr);

    // ===================================================
    // 3. Blocked Matrix Multiplication 
    // ===================================================
    matrixMultiplyWorker(flatZS.get(), flatZS_transposed.get(), dstMatrix, 0, dim, dim, nullptr);

#endif
}

// =================================================================================
// Worker: 2D to 1D Array Mapper
// Reorganizes 2D vector matrix into a 1D array 
// to maximise cache hits for subsequent operations
// =================================================================================
void flattenWorker(const std::vector<std::vector<double>>* srcMatrix, double* flatSrc, 
    const int startRow, const int endRow, 
    const int dim, 
    barrier* syncBarrier)
{
    for (int i = startRow; i < endRow; i++)
    {
        // Grab the row reference to avoid pointer chasing
        const std::vector<double>& row = (*srcMatrix)[i];

        // Pre-calculate the base index
        const int baseIndex = i * dim;

        for (int j = 0; j < dim; j++)
        {
            // Populate the flat array
            flatSrc[baseIndex + j] = row[j];
        }
    }

    // Decrement the barrier count and wait for the main thread to catch up
    if (syncBarrier != nullptr)
    {
        syncBarrier->count_down_and_wait();
    }
}

// =================================================================================
// Worker: Fused Zone Sum & Transpose
// Computes a 3x3 zone sum outputting a standard 
// and transposed matrix, setting up the multiplication phase
// =================================================================================
void zoneSumTransposeWorker(const double* flatSrc, double* flatZS, double* flatZS_transposed, 
    const int startRow, const int endRow, 
    const int dim, 
    barrier* syncBarrier)
{
    for (int i = startRow; i < endRow; i++) // Row
    {
        // Pre-calculate the starting index for writes
        const int destRowStart = i * dim;

        for (int j = 0; j < dim; j++) // Column
        {
            // Invert coordinates to simulate reading from a transposed matrix
            const int transposeRow = j;
            const int transposeCol = i;

            // Clamp the 3x3 boundaries to prevent segmentation faults
            const int minRow = std::max(0, transposeRow - 1);
            const int maxRow = std::min(dim - 1, transposeRow + 1);
            const int minCol = std::max(0, transposeCol - 1);
            const int maxCol = std::min(dim - 1, transposeCol + 1);

            double sum = 0.0;

            // Accumulate the sum of the 3x3 zone
            for (int r = minRow; r <= maxRow; r++)
            {
                const int srcRowStart = r * dim; // Find where the current zone row starts in memory
                for (int c = minCol; c <= maxCol; ++c)
                {
                    sum += flatSrc[srcRowStart + c];
                }
            }

            flatZS[destRowStart + j] = sum; // Base matrix for the upcoming multiplication
            flatZS_transposed[j * dim + i] = sum; // Transposed matrix to align rows for dot-products
        }
    }

    // Decrement the barrier count and wait
    if (syncBarrier != nullptr)
    {
        syncBarrier->count_down_and_wait();
    }
}

// =================================================================================
// Helper: Matrix Multiplication Core Logic
// Processes a specific tile. 'inline' embeds this directly, 
// preventing function-call overhead
// =================================================================================
inline void matrixMultiplyHelper(const double* flatZS, const double* flatZS_transposed, std::vector<std::vector<double>>* dstMatrix,
    const int iTile, const int iMax,
    const int jTile, const int jMax,
    const int kTile, const int kMax,
    const int dim)
{
    // Iterate through the rows of the current tile
    for (int i = iTile; i < iMax; i++)
    {
        // Extract raw pointer to the destination row
        double* destRow = (*dstMatrix)[i].data();

        // Calculate the base pointer for the current row in the source matrix
        const double* rowA = &flatZS[i * dim];

        int j = jTile; // Column tracking

        // Calculate 4 output columns simultaneously
        // This improves the compute to memory fetch ratio by reusing loaded data
        for (; j <= jMax - 4; j += 4)
        {
            // Get pointers to the 4 corresponding columns stored as rows in the transposed matrix
            const double* rowB0 = &flatZS_transposed[(j + 0) * dim];
            const double* rowB1 = &flatZS_transposed[(j + 1) * dim];
            const double* rowB2 = &flatZS_transposed[(j + 2) * dim];
            const double* rowB3 = &flatZS_transposed[(j + 3) * dim];

            // Initialise accumulators. Keeping these local causes the compiler to use CPU registers
            double sum0 = 0.0, sum1 = 0.0, sum2 = 0.0, sum3 = 0.0;

            // Compute the dot product for this specific tile segment
            for (int k = kTile; k < kMax; k++)
            {
                // Load the value from Matrix A once from the L1 cache
                double a = rowA[k];

                // Reuse the loaded value 'a' 4 times in the registers
                sum0 += a * rowB0[k];
                sum1 += a * rowB1[k];
                sum2 += a * rowB2[k];
                sum3 += a * rowB3[k];
            }

            // Direct sequential writes to the destination matrix
            destRow[j + 0] += sum0;
            destRow[j + 1] += sum1;
            destRow[j + 2] += sum2;
            destRow[j + 3] += sum3;
        }

        // Edge Case Cleanup: Handle any remaining columns if the tile width isn't a multiple of 4
        for (; j < jMax; j++)
        {
            const double* rowB = &flatZS_transposed[j * dim];
            double sum = 0.0;
            for (int k = kTile; k < kMax; k++)
            {
                sum += rowA[k] * rowB[k];
            }
            destRow[j] += sum;
        }
    }
}

// =================================================================================
// Worker: Tiled Matrix Multiplication
// Multiplies flatZS by flatZS_transposed using L1 cache-sized tiles
// writing the result directly into the final dstMatrix.
// =================================================================================
void matrixMultiplyWorker(const double* flatZS, const double* flatZS_transposed, std::vector<std::vector<double>>* dstMatrix, 
    const int startRow, const int endRow, 
    const int dim, 
    barrier* syncBarrier)
{
    // Define the tile size to fit data within the CPU's L1 cache 
    const int tile_size = 64;

    // Zero the destination matrix 
    for (int i = startRow; i < endRow; i++)
    {
        std::fill((*dstMatrix)[i].begin(), (*dstMatrix)[i].end(), 0.0);
    }

    // Iterate over the matrix in tiles
    for (int iTile = startRow; iTile < endRow; iTile += tile_size) // Row coordinate
    {
        // Clamp the row tile to prevent out-of-bounds access
        const int iMax = std::min(iTile + tile_size, endRow);

        for (int jTile = 0; jTile < dim; jTile += tile_size) // Col coordinate
        {
            // Clamp the column tile boundary
            const int jMax = std::min(jTile + tile_size, dim);

            for (int kTile = 0; kTile < dim; kTile += tile_size) // Dot product 
            {
                // Clamp the dot-product tile boundary
                const int kMax = std::min(kTile + tile_size, dim);

                // Pass the tile dimensions to the compute helper
                matrixMultiplyHelper(flatZS, flatZS_transposed, dstMatrix,
                    iTile, iMax, jTile, jMax, kTile, kMax, dim);
            }
        }
    }

    // Decrement the barrier count and wait
    if (syncBarrier != nullptr)
    {
        syncBarrier->count_down_and_wait();
    }
}