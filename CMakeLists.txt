# Definition of the minimum required cmake Version
cmake_minimum_required(VERSION 3.0)
# Definition of the Project
# Later you can access the project variable like ${CFDLAB_SOURCE_DIR}
project(CFDLAB VERSION 1.0 LANGUAGES CXX CUDA)

find_package(CUDA REQUIRED)

# Define all configuration options
option(gpp9 "compile with gpp9 filesystem" ON)

# Definition of the C++ Standard 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-O3")

set(CUDA_ARCHITECTURES 61 CACHE STRING "CUDA architectures to compile for")
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)
#set(gpp9 True)


# Creating the executable of our project and the required dependencies
# the executable is called fluidchen
file(GLOB files src/*.cpp)
set(SOURCES_CPP
    src/main.cpp
    src/PressureSolver.cpp
    src/Grid.cpp
    src/Fields.cpp
    src/Discretization.cpp
    src/Communication.cpp
    src/Cell.cpp
    src/Case.cpp
    src/Boundary.cpp
)

set(SOURCES_CU
    src/ThrustTrial.cu
)

##add_executable(fluidchen ${files})
add_executable(fluidchen ${SOURCES_CPP} ${SOURCES_CU})

target_compile_definitions(fluidchen PUBLIC -Dsolution_liddriven)
target_compile_definitions(fluidchen PUBLIC -Dsolution_energy)
target_compile_definitions(fluidchen PUBLIC -Dsolution_parallelization)

set_target_properties(fluidchen PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    CUDA_ARCHITECTURES "61"
)


# You can find package likes
# find_package(MPI)
# Require a package
find_package(MPI REQUIRED)
# Find a package with different components e.g. BOOST
# find_package(Boost COMPONENTS filesystem REQUIRED)

# VTK Library
find_package(VTK REQUIRED)
message (STATUS "VTK_VERSION: ${VTK_VERSION}")
include(${VTK_USE_FILE})

# Filesystem library is only available since GCC 9
# (if you are using a different compiler, comment-out these lines -- contributions welcome!)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9")
    message("g++ Version is lower than Version 9")
    if (NOT APPLE)  
      target_link_libraries(fluidchen PUBLIC stdc++fs)
    endif()
    else()
    message("g++ Version is 9 or higher")
    target_compile_definitions(fluidchen PUBLIC gpp9)
    target_compile_definitions(fluidchen PUBLIC -DGCC_VERSION_9_OR_HIGHER)
  endif()
endif()

# Add include directory
target_include_directories(fluidchen PUBLIC include)
target_include_directories(fluidchen PRIVATE ${CUDA_TOOLKIT_ROOT_DIR}/include)

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX /usr/local)
endif()

# if you use external libraries you have to link them like
target_link_libraries(fluidchen PRIVATE MPI::MPI_CXX)
target_link_libraries(fluidchen PRIVATE ${VTK_LIBRARIES})
target_link_libraries(fluidchen PRIVATE ${CUDA_LIBRARIES})

install(TARGETS fluidchen DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
# If you write tests, you can include your subdirectory (in this case tests) as done here
# Testing
