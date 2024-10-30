#include <gmp.h>
#define __gmp_const const

#include "NN_axi.h"

#include <ap_utils.h>

bool load(input_axi_t &in, input_t in_local[N_IQ_WINDOW_IN*2], unsigned *scaling_factor) {
	#pragma HLS INLINE
    // Read readout data
    LOAD_L: for(unsigned i = 0, j = 1; i < 2*N_IQ_WINDOW_IN; i+=2, j+=2) {
		#pragma HLS PIPELINE II=1 rewind
    	ap_uint<32> data_in;
    	in.read(data_in);
    	input_t lo = data_in.range(13,0); // * *scaling_factor;
    	input_t hi = data_in.range(29,16); // * *scaling_factor;
   		in_local[i] = lo;
   		in_local[j] = hi;
    }

    return true;
}

bool store(result_t out_local[N_OUT], output_axi_t out[BUFFER_SIZE], unsigned &k) {
	#pragma HLS INLINE
	// Output logits (ground [0], excited [1])
	STORE_L: for(unsigned i = 0; i < N_OUT; i++){
#pragma HLS UNROLL
    	//#pragma HLS PIPELINE rewind
    	out[k*2 + i] = out_local[i];
	}

	return true;
}

void NN_axi(input_axi_t &in, output_axi_t out[BUFFER_SIZE], bool trigger, unsigned *window_size, unsigned *window_offset, unsigned *scaling_factor, unsigned *out_reset, unsigned *out_offset) {

    // Unregistered axis
	#pragma HLS INTERFACE axis off port=in
	// Registered axis
	//#pragma HLS INTERFACE axis register both port=in
    #pragma HLS INTERFACE bram depth=4294967295 latency=1 port=out
    #pragma HLS INTERFACE ap_none port=trigger
	#pragma HLS INTERFACE s_axilite register port=window_size bundle=config
	#pragma HLS INTERFACE s_axilite register port=window_offset bundle=config
    #pragma HLS INTERFACE s_axilite register port=scaling_factor bundle=config
	#pragma HLS INTERFACE s_axilite register port=out_reset bundle=config
    #pragma HLS INTERFACE s_axilite register port=out_offset bundle=config
    #pragma HLS INTERFACE ap_ctrl_none port=return


	// I/O buffers of the hls4ml NN module
    input_t in_local[N_IQ_WINDOW_IN*2];
    result_t out_local[N_OUT];

    // Index of the output buffer over AXI-lite / MMIO
    unsigned k = 0;
#ifdef __SYNTHESIS__
    // Always active
    FOREVER_L: do {
#endif

    	// Reset output buffer over AXI-lite / MMIO
        OUT_RESET_C: if ((*out_reset) == 255) {
            k = 0;
            *out_offset = 0;
        }

        // Trigger for readout data
        TRIGGER_C: if (trigger) {

        	bool load_done = false;
            bool buffer_ff_done = false;
        	bool nn_done = false;

        	// If you need you can wait extra clock cycles
            WINDOW_OFFSET_L: ap_wait_n(*window_offset);

            load_done = load(in, in_local, scaling_factor);

            // hls4ml NN module
        	nn_done = NN(in_local, out_local);

        	store(out_local, out, k);

        	// Increment and reset index of the output buffer over AXI-lite / MMIO
        	k++;
            if (k*2 >= BUFFER_SIZE)
                k = 0;

            // Keep track of the current index via AXI-lite / MMIO
            *out_offset = k;
        }
#ifdef __SYNTHESIS__
    } while (true);
#endif
}

