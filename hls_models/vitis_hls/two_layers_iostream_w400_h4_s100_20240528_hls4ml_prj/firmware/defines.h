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
#define N_LAYER_2 4
#define N_LAYER_5 1


// hls-fpga-machine-learning insert layer-precision
typedef ap_fixed<14,14> input_t;
typedef ap_fixed<19,18> fc1_accum_t;
typedef ap_fixed<19,18> layer2_t;
typedef ap_fixed<6,1> weight2_t;
typedef ap_fixed<6,1> bias2_t;
typedef ap_uint<1> layer2_index;
typedef ap_fixed<19,18> layer3_t;
typedef ap_fixed<18,8> fc1_relu_table_t;
typedef ap_fixed<24,8> batchnorm1_accum_t;
typedef ap_fixed<24,8> layer4_t;
typedef ap_fixed<24,8> batchnorm1_scale_t;
typedef ap_fixed<24,8> batchnorm1_bias_t;
typedef ap_fixed<10,5> fc2_accum_t;
typedef ap_fixed<10,5> result_t;
typedef ap_fixed<6,1> weight5_t;
typedef ap_fixed<6,1> bias5_t;
typedef ap_uint<1> layer5_index;


#endif
