//Numpy array shape [1]
//Min 1.000000000000
//Max 1.000000000000
//Number of zeros 0

#ifndef S8_H_
#define S8_H_

#ifndef __SYNTHESIS__
exponent_scale8_t s8[1];
#else
exponent_scale8_t s8[1] = {{1.0, 0}};

#endif

#endif
