/*
In this file, we define our main function and ensure that a valid input
date file is provided.
*/
#include <mpi.h>
//#include <thrust/device_vector.h>
//#include <thrust/host_vector.h>

#include <iostream>
#include <string>

#include "Case.hpp"
#include "Communication.hpp"
//#include <thrust/device_vector.h>
#include "ThrustTrial.hpp"

int main(int argn, char **args) {
    const size_t N = 100;
    
    // Create a ThrustVector with N elements
    ThrustVector tvec(N);

    // Fill the vector with the value 1.0f
    tvec.fill(1.0f);

    // Copy the data back to host
    std::vector<float> host_data = tvec.copyToHost();

    // Display the first 10 elements
    for (size_t i = 0; i < 10; ++i) {
        std::cout << host_data[i] << " ";
    }
    std::cout << std::endl;
  /* thrust::device_vector<int> d_vec(5); */
  /* Communication::init_parallel(argn, args); */
  /* if (argn > 1) { */
  /*   std::string file_name{args[1]}; */
  /*   Case problem(file_name, argn, args); */
  /*   problem.simulate(); */
  /* } else { */
  /*   std::cout << "Error: No input file is provided to fluidchen." << std::endl; */
  /*   std::cout << "Example usage: /path/to/fluidchen /path/to/input_data.dat" */
  /*             << std::endl; */
  /* } */
  /* Communication::finalize(); */
}
