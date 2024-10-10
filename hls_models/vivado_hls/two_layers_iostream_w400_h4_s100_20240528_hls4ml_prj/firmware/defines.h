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
#define N_LAYER_5 1

#define FIFO_IN_DEPTH 2

// hls-fpga-machine-learning insert layer-precision
typedef nnet::array<ap_fixed<14,14>, FIFO_IN_DEPTH> input_t;
typedef ap_fixed<18,17> fc1_accum_t;
typedef nnet::array<ap_fixed<18,17>, 4*1> layer2_t;
typedef ap_fixed<6,1> weight2_t;
typedef ap_fixed<6,1> bias2_t;
typedef ap_uint<1> layer2_index;
typedef nnet::array<ap_fixed<18,17>, 4*1> layer3_t;
typedef ap_fixed<18,8> fc1_relu_table_t;
typedef ap_fixed<24,4> batchnorm1_accum_t;
typedef nnet::array<ap_fixed<24,4>, 4*1> layer4_t;
typedef ap_fixed<24,4> batchnorm1_scale_t;
typedef ap_fixed<24,4> batchnorm1_bias_t;
typedef ap_fixed<8,5> fc2_accum_t;
typedef nnet::array<ap_fixed<8,5>, 1*1> layer5_t;
typedef ap_fixed<6,1> weight5_t;
typedef ap_fixed<6,1> bias5_t;
typedef ap_uint<1> layer5_index;
typedef nnet::array<ap_fixed<8,1>, 1*1> result_t;
typedef ap_fixed<18,8> fc2_sigmoid_table_t;

#endif
