#include "ThrustTrial.hpp"

// Constructor
ThrustVector::ThrustVector(size_t size)
    : d_vector(size) {
}

// Destructor
ThrustVector::~ThrustVector() {
    // No need for manual memory management, as thrust::device_vector handles it
}

// Fill the device vector with a specific value
void ThrustVector::fill(float value) {
    thrust::fill(d_vector.begin(), d_vector.end(), value);
}

// Copy data from host to device
void ThrustVector::copyFromHost(const std::vector<float>& host_data) {
    d_vector = host_data;  // Thrust automatically handles the copy
}

// Copy data from device to host
std::vector<float> ThrustVector::copyToHost() const {
    std::vector<float> host_data(d_vector.size());
    thrust::copy(d_vector.begin(), d_vector.end(), host_data.begin());
    return host_data;
}

// Get the size of the vector
size_t ThrustVector::size() const {
    return d_vector.size();
}

// Access to the underlying device vector
thrust::device_vector<float>& ThrustVector::getDeviceVector() {
    return d_vector;
}
