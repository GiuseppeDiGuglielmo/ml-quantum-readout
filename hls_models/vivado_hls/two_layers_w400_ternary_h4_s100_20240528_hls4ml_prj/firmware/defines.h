#ifndef DEFINES_H_
#define DEFINES_H_

#include "ap_fixed.h"
#include "ap_int.h"
#include "nnet_utils/nnet_types.h"
#include <cstddef>
#include <cstdio>

// hls-fpga-machine-learning insert numbers
#define N_INPUT_1_1 800
#define N_LAYER_2 4
#define N_LAYER_2 4
#define N_LAYER_5 1
#define N_LAYER_5 1


// hls-fpga-machine-learning insert layer-precision
typedef ap_fixed<14,14> input_t;
typedef ap_fixed<20,20> fc1_accum_t;
typedef ap_fixed<20,20> layer2_t;
typedef ap_int<2> weight2_t;
typedef ap_int<2> bias2_t;
typedef ap_uint<1> layer2_index;
typedef ap_fixed<20,20> layer7_t;
typedef struct exponent_scale7_t {ap_uint<1> sign;ap_int<4> weight; } exponent_scale7_t;
typedef ap_int<2> bias7_t;
typedef ap_fixed<21,21> fc2_accum_t;
typedef ap_fixed<21,21> layer5_t;
typedef ap_int<2> weight5_t;
typedef ap_int<2> bias5_t;
typedef ap_uint<1> layer5_index;
typedef ap_fixed<21,21> result_t;
typedef struct exponent_scale8_t {ap_uint<1> sign;ap_int<2> weight; } exponent_scale8_t;
typedef ap_int<2> bias8_t;


#endif
