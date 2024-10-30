#include <gmp.h>
#define __gmp_const const

#include <iostream>

#include "NN.h"
#include "parameters.h"


bool NN(
    input_t fc1_input[N_INPUT_1_1],
    result_t layer8_out[N_LAYER_5]
) {

    // hls-fpga-machine-learning insert IO
    #pragma HLS ARRAY_RESHAPE variable=fc1_input complete dim=0
    #pragma HLS ARRAY_PARTITION variable=layer8_out complete dim=0
    #pragma HLS INTERFACE ap_vld port=fc1_input,layer8_out 
    #pragma HLS DATAFLOW 

    // hls-fpga-machine-learning insert load weights
#ifndef __SYNTHESIS__
    static bool loaded_weights = false;
    if (!loaded_weights) {
        nnet::load_weights_from_txt<weight2_t, 3200>(w2, "w2.txt");
        nnet::load_weights_from_txt<bias2_t, 4>(b2, "b2.txt");
        nnet::load_exponent_weights_from_txt<exponent_scale7_t, 4>(s7, "s7.txt");
        nnet::load_weights_from_txt<bias7_t, 4>(b7, "b7.txt");
        nnet::load_weights_from_txt<weight5_t, 4>(w5, "w5.txt");
        nnet::load_weights_from_txt<bias5_t, 1>(b5, "b5.txt");
        nnet::load_exponent_weights_from_txt<exponent_scale8_t, 1>(s8, "s8.txt");
        nnet::load_weights_from_txt<bias8_t, 1>(b8, "b8.txt");
        loaded_weights = true;    }
#endif
    // ****************************************
    // NETWORK INSTANTIATION
    // ****************************************

    // hls-fpga-machine-learning insert layers

    layer2_t layer2_out[N_LAYER_2];
    #pragma HLS ARRAY_PARTITION variable=layer2_out complete dim=0
    nnet::dense<input_t, layer2_t, config2>(fc1_input, layer2_out, w2, b2); // fc1

    layer7_t layer7_out[N_LAYER_2];
    #pragma HLS ARRAY_PARTITION variable=layer7_out complete dim=0
    nnet::normalize<layer2_t, layer7_t, config7>(layer2_out, layer7_out, s7, b7); // fc1_alpha

    layer5_t layer5_out[N_LAYER_5];
    #pragma HLS ARRAY_PARTITION variable=layer5_out complete dim=0
    nnet::dense<layer7_t, layer5_t, config5>(layer7_out, layer5_out, w5, b5); // fc2

    nnet::normalize<layer5_t, result_t, config8>(layer5_out, layer8_out, s8, b8); // fc2_alpha

    return true;
}

