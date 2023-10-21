cmake_minimum_required(VERSION 3.10)
project(rsteg)

message("Setting up CMake for rsteg project.")

set(CMAKE_CXX_STANDARD 20)
message("C++ standard set to C++20.")

set(SRC
    io_helpers.hpp
    aes_helpers.hpp
    lsb_rand.hpp
    rsteg.cpp
)

add_executable(rsteg ${SRC})
message("Creating executable 'rsteg'.")

set_target_properties(rsteg PROPERTIES OUTPUT_NAME "rsteg")
message("Setting the output name to 'rsteg'.")

if(UNIX)
    # Unix
    find_package(OpenSSL REQUIRED)
    find_package(PNG REQUIRED)
    find_package(OpenCV REQUIRED)
    include_directories(${OpenCV_INCLUDE_DIRS})
    target_link_libraries(rsteg PRIVATE OpenSSL::SSL OpenSSL::Crypto PNG::PNG ${OpenCV_LIBS})
    message("Configuring for Unix platform.")

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
        message("Debug Build on Unix.")
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
        message("Release Build on Unix.")
    endif()

else()
    # Windows
    find_package(OpenCV REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(PNG REQUIRED)
    include_directories(${OpenCV_INCLUDE_DIRS})
    target_link_libraries(rsteg PRIVATE -lssl -lcrypto -lpng ${OpenCV_LIBS})
    message("Configuring for Windows platform.")

    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug")
        message("Debug Build on Windows.")
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release")
        message("Release Build on Windows.")
    endif()
    
endif()
