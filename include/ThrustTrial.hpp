#ifndef THRUST_VECTOR_H
#define THRUST_VECTOR_H

#include <thrust/device_vector.h>
#include <thrust/host_vector.h>

template <typename T>
class MatrixThrust {
  public:
    // Constructor that initializes the device vector with a given size
    MatrixThrust<T>() = default;

    // Destructor
    ~MatrixThrust();

    /**
     * @brief Constructor for Thrust Matrix with initial value
     *
     * @param[in] number of elements in x direction
     * @param[in] number of elements in y direction
     * @param[in] initial value for the elements
     */
    MatrixThrust<T>(int i_max, int j_max, double init_val) : _imax(i_max), _jmax(j_max) {
        h_vector.resize(i_max * j_max);
        d_vector.resize(i_max * j_max);
    }


  private:
    thrust::device_vector<float> d_vector; // Device vector for GPU
    thrust::host_vector<float> h_vector;   // Host vector for CPU
    int _imax;
    int _jmax;
    //int _container;
};

#endif // THRUST_VECTOR_H
