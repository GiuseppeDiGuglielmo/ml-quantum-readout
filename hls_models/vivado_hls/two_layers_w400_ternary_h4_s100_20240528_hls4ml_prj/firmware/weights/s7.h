//Numpy array shape [1]
//Min 0.125000000000
//Max 0.125000000000
//Number of zeros 0

#ifndef S7_H_
#define S7_H_

#ifndef __SYNTHESIS__
exponent_scale7_t s7[4];
#else
exponent_scale7_t s7[4] = {{1.0, -15}, {1.0, -15}, {1.0, -15}, {1.0, -15}};

#endif

#endif
