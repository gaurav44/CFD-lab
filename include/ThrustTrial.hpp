#ifndef THRUST_VECTOR_H
#define THRUST_VECTOR_H

#include <thrust/host_vector.h>
#include <thrust/device_vector.h>

class ThrustVector {
public:
    // Constructor that initializes the device vector with a given size
    ThrustVector(size_t size);

    // Destructor
    ~ThrustVector();

    // Method to fill the vector with a specific value
    void fill(float value);

    // Method to copy data from host to device
    void copyFromHost(const std::vector<float>& host_data);

    // Method to copy data from device to host
    std::vector<float> copyToHost() const;

    // Method to get the size of the vector
    size_t size() const;

    // Access to the underlying device vector
    thrust::device_vector<float>& getDeviceVector();

private:
    thrust::device_vector<float> d_vector;  // Device vector for GPU
};

#endif // THRUST_VECTOR_H
