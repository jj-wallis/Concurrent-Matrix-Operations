#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <limits>
#include <ios>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <thread>

#include "RandomMatrixGenerator.h"
#include "FileRead.h"
#include "MatrixOperations.h"
#include "FileWrite.h"

int main()
{
    // =================================================================================
    // Application Header & Mode Readout
    // =================================================================================
    std::cout << "==================================================\n";
    std::cout << "           CONCURRENT MATRIX OPERATIONS           \n";
    std::cout << "==================================================\n";

#if PARALLEL_MODE
    std::cout << "Mode: MULTITHREADED (Parallel)\n";
    std::cout << "Hardware Concurrency: " << std::thread::hardware_concurrency() << " Threads Detected\n";
#else
    std::cout << "Mode: SEQUENTIAL (Baseline)\n";
    std::cout << "Hardware Concurrency: Single Thread Execution\n";
#endif
    std::cout << "--------------------------------------------------\n";

    // =================================================================================
    // Data Initialization
    // =================================================================================
    int dim = 0;
    std::string readFromFileInput;
    std::vector<std::vector<double>> srcMatrix;

    std::cout << "Do you want to read the matrix from a file (y/n): ";
    std::cin >> readFromFileInput;

    if (readFromFileInput == "y" || readFromFileInput == "Y")
    {
        std::string fileName;
        std::cout << "Enter the file name: ";
        std::cin >> fileName;

        // Auto-append .txt if the user didn't type it
        if (fileName.length() < 4 || fileName.substr(fileName.length() - 4) != ".txt")
        {
            fileName += ".txt";
        }

        // Read from the file
        srcMatrix = fileRead(fileName);
        dim = srcMatrix.size();

        if (dim == 0) {
            std::cout << "[ERROR] Matrix reading failed or file is empty. Exiting program.\n";
            return 1;
        }
    }
    else
    {
        std::string input;

        // Ask user for matrix size with robust validation
        do {
            dim = 0;
            input = "";

            std::cout << "Enter matrix size greater than 0: ";
            std::cin >> input;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            try
            {
                dim = std::stoi(input);
            }
            catch (std::invalid_argument&)
            {
                std::cout << "[ERROR] Invalid input. Please enter a positive integer.\n";
                continue;
            }
            catch (std::out_of_range&)
            {
                std::cout << "[ERROR] Input is out of range. Please enter a positive integer smaller than " << INT_MAX << ".\n";
                continue;
            }

            if (dim <= 0)
            {
                std::cout << "[ERROR] Invalid input. Please enter a positive integer.\n";
            }

        } while (dim <= 0);

        // Generate a random matrix of the specified size
        srcMatrix = randomMatrixGenerator(dim);
    }

    // =================================================================================
    // Execution & Benchmarking
    // =================================================================================
    std::cout << "\n[INFO] Initializing " << dim << " x " << dim << " matrices...\n";
    std::cout << "[INFO] Running 10 benchmark iterations...\n\n";

    std::vector<std::vector<double>> finalMatrix;
    double totalTime = 0.0;
    const int iterations = 10;

    for (int i = 1; i <= iterations; i++)
    {
        // Clear and reinitialise the final matrix
        finalMatrix.clear();
        finalMatrix.resize(dim);
        for (int j = 0; j < dim; j++)
        {
            finalMatrix[j].resize(dim);
        }

        // Start chronometer
        std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();

        // Perform the operations
        matrixOperationsInit(&srcMatrix, &finalMatrix);

        // Stop chronometer
        std::chrono::time_point<std::chrono::high_resolution_clock> stop = std::chrono::high_resolution_clock::now();

        // Calculate and accumulate interval
        double interval = (std::chrono::duration<double, std::milli>(stop - start)).count();
        totalTime += interval;

        // Print formatted runtime dynamically
        std::cout << "Run " << std::setw(2) << i << "/" << iterations
            << " ... " << std::fixed << std::setprecision(2) << interval << " ms\n";
    }

    // =================================================================================
    // Results & File Output
    // =================================================================================
    double averageTime = totalTime / iterations;

    std::cout << "\n--------------------------------------------------\n";
    std::cout << ">> FINAL AVERAGE: " << std::fixed << std::setprecision(2) << averageTime << " ms\n";
    std::cout << "--------------------------------------------------\n";

    std::string writeChoice;
    std::cout << "Do you want to write the initial and final matrices to file? (y/n): ";
    std::cin >> writeChoice;

    if (writeChoice == "y" || writeChoice == "Y")
    {
        std::string fileNamePrefix;
        std::cout << "Enter the file name (without extension): ";
        std::cin >> fileNamePrefix;

        // Save both matrices
        fileWrite(fileNamePrefix + "_src.txt", &srcMatrix);
        fileWrite(fileNamePrefix + "_dst.txt", &finalMatrix);

        std::cout << "[SUCCESS] Files successfully written.\n";
    }

    return 0;
}