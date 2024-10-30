#ifndef NN_AXI_H_
#define NN_AXI_H_

#include "NN.h"
#include "hls_stream.h"

#include <iostream>

//static const unsigned N_IQ_IN = 770; // 770 (I,Q)
static const unsigned N_IQ_WINDOW_IN = 400;
static const unsigned N_OUT = 1;
//static const unsigned N_CONFIGS = 2;
static const unsigned BUFFER_OUT = N_OUT * 16384; // 1 * 16384 * 4B = 64KB

typedef hls::stream< ap_uint<32> > input_axi_t;
typedef float output_axi_t;
typedef float output_t;

void NN_axi(
        input_axi_t &in,
        output_axi_t out[BUFFER_OUT],
        bool trigger,
        unsigned *window_size,
        unsigned *window_offset,
        unsigned *scaling_factor,
        unsigned *trigger_delay,
        unsigned *ou_buffer_head
    );

#endif
