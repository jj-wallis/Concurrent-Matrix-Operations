#include <iostream>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <string>
#include <limits>
#include <ios>
#include <cctype>
#include <filesystem>

#include "RandomMatrixGenerator.h"
#include "FileRead.h"
#include "MatrixOperations.h"
#include "FileWrite.h"

int main()
{
    // Declare matrix size parameter
    int dim = 0;
    std::string readFromFileInput;
    std::string input;

    std::vector<std::vector<double>> srcMatrix;

    // Ask to read from file
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

        // Check the matrix was succesfully populated
        if (dim == 0) {
            std::cout << "Matrix reading failed or file is empty. Exiting program." << std::endl;
            return 1;
        }
    }
    
    else 
    {
    // Asks the user the size of a random matrix
    // At the end of this block the srcMatrix variable will contain the matrix to be used in the operations
    // And the dim variable will contain the size of the matrix
        std::string input;

        // Ask user for matrix size
        do {
            dim = 0;
            input = ""; // Reset input string for each iteration

            std::cout << "Enter matrix size greater than 0: ";
            std::cin >> input;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Ignore any leftover characters in the input buffer

            // Check if the input is a valid integer
            try
            {
                dim = std::stoi(input);
            }
            catch (std::invalid_argument&)
            {
                std::cout << "Invalid input. Please enter a positive integer." << std::endl;
                continue; // Skip to the next iteration of the loop
            }
            catch (std::out_of_range&)
            {
                std::cout << "Input is out of range. Please enter a positive integer smaller than " << INT_MAX << "." << std::endl;
                continue; // Skip to the next iteration of the loop
            }

            // Check if the input is a positive integer
            if (dim <= 0)
            {
                std::cout << "Invalid input. Please enter a positive integer." << std::endl;
            }

        } while (dim <= 0); // Ensure the input is a positive integer

        // Generate a random matrix of the specified size
        srcMatrix = randomMatrixGenerator(dim); //generate random matrix
    }

    // Print out matrix size for user knowledge
    std::cout << "Matrix size: " << dim << std::endl;

    // Declare final matrix to store data after computations
    std::vector<std::vector<double>> finalMatrix;

    // Declare a vector to store time intervals from measurements
    std::vector<double> timeIntervals;

    // Repeat the operations 10 times to get an averaged execution time
    for (int i = 0; i < 10; i++)
    {
        // Clear and reinitialise the final matrix to ensure it is empty before each iteration
        finalMatrix.clear();
        finalMatrix.resize(dim); // Resize the final matrix to the same size as the source matrix
        for (int j = 0; j < dim; j++)
        {
            finalMatrix[j].resize(dim); // Resize each row of the final matrix to the same size as the source matrix
        }

        // Print the iteration count
        std::cout << "Iteration: " << i + 1 << std::endl;

        // Start the chronometer to measure execution time
        std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();

        // Perform the operations
        matrixOperationsInit(&srcMatrix, &finalMatrix);

        // Stop the chronometer at the end of the operations to capture the execution time
        std::chrono::time_point<std::chrono::high_resolution_clock> stop = std::chrono::high_resolution_clock::now();

        // The execution time is computed as final time minus start time
        double interval = (std::chrono::duration<double, std::milli>(stop - start)).count();

        // The time interval is queued in a vector to allow for the computation of an average
        timeIntervals.push_back(interval);
    }

    // Compute the average of the time intervals measured earlier
    // Set a cumulative variable to 0 to sum all the intervals
    double sum = 0;

    // Loop through all the intervals in the vector and sum them in the cumulative variable above
    for (int i = 0; i < timeIntervals.size(); i++)
    {
        std::cout << timeIntervals[i] << " ms" << std::endl;
        sum += timeIntervals[i];
    }

    // Print the average value of all the intervales
    std::cout << "Average: " << sum / timeIntervals.size() << " ms" << std::endl;

    // Ask to write to file
    std::string writeChoice;
    std::cout << "Do you want to write the initial and final matrices to file? (y/n): ";
    std::cin >> writeChoice;

    if (writeChoice == "y" || writeChoice == "Y")
    {
        std::string fileNamePrefix;
        std::cout << "Enter the file name (without extension): ";
        std::cin >> fileNamePrefix;

        // Save both matrices.
        fileWrite(fileNamePrefix + "_src.txt", &srcMatrix);
        fileWrite(fileNamePrefix + "_dst.txt", &finalMatrix);

        std::cout << "Files successfully written." << std::endl;
    }

    // Terminate execution without errors
    return 0;

    // Terminate execution without errors
    return 0;
}
