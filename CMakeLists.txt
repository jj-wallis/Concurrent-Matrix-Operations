cmake_minimum_required(VERSION 3.15)

# Define the project name and specify C++ as the language
project(ConcurrentMatrixOperations VERSION 1.0.0 LANGUAGES CXX)

# Enforce modern C++ standards
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Gather all C++ source files
file(GLOB SOURCES "*.cpp")

# Create the executable target
add_executable(matrix_operations ${SOURCES})

# Automatically locate and link the correct OS-level threading library (-pthread)
find_package(Threads REQUIRED)
target_link_libraries(matrix_operations PRIVATE Threads::Threads)

# If compiling on Linux (GCC/Clang), safely apply the high-performance flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(matrix_operations PRIVATE -O3 -march=native)
endif()
