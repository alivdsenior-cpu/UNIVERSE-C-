cmake_minimum_required(VERSION 3.15)
project(ApexTrade VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for performance and warnings
if(MSVC)
    add_compile_options(/W4 /WX /O2)
else()
    add_compile_options(-Wall -Wextra -Werror -O3 -march=native)
endif()

# Includes
include_directories(include)

# Executable
add_executable(apextrade 
    src/main.cpp
    src/OrderBook.cpp
)
