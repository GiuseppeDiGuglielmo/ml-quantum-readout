#include <gmp.h>
#define __gmp_const const

#include "NN_axi.h"

#include <ap_utils.h>
#include <utils/x_hls_utils.h>

// Reads offset + N_IQ_WINDOW_IN samples, one per cycle; the shift register
// keeps the last N_IQ_WINDOW_IN, so the window starts offset samples after
// the first read (window_offset counts samples)
bool load(input_axi_t &in, input_t in_local[N_IQ_WINDOW_IN*2], unsigned offset, unsigned *scaling_factor) {
    #pragma HLS INLINE
    // Read readout data
    // No rewind: it made HLS drop the pipeline (NN() is a dataflow region
    // in the same FOREVER_L body), so LOAD_L ran at II=3
    LOAD_L: for(unsigned i = 0; i < offset + N_IQ_WINDOW_IN; i++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=400 max=770
        ap_uint<32> data_in;
        in.read(data_in);
        input_t lo = data_in.range(13,0); // * *scaling_factor;
        input_t hi = data_in.range(29,16); // * *scaling_factor;
        // Shift the window down by one (I,Q) pair and append the new one:
        // fixed wiring per element instead of an indexed write into the
        // reshaped array, which fanned each sample out to all 400 slots
        SHIFT_L: for(unsigned n = 0; n < 2*N_IQ_WINDOW_IN - 2; n++) {
            #pragma HLS UNROLL
            in_local[n] = in_local[n+2];
        }
        in_local[2*N_IQ_WINDOW_IN - 2] = lo;
        in_local[2*N_IQ_WINDOW_IN - 1] = hi;
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

// Wait for the trigger or an out_reset request, checking both every cycle
// (II=1), so the window starts a fixed number of cycles after the trigger
// edge. A separate module (INLINE off): HLS drops PIPELINE on a loop in
// FOREVER_L, because FOREVER_L also holds the NN dataflow region
void wait_trigger(volatile bool &trigger, volatile unsigned *out_reset, bool &trig, bool &reset) {
    #pragma HLS INLINE off
    bool t, r;
    TRIG_WAIT_L: do {
        #pragma HLS PIPELINE II=1
        t = trigger;
        r = (*out_reset == 255);
    } while (!t && !r);
    trig = t;
    reset = r;
}

// trigger is a volatile reference so HLS re-reads the ap_none port on every
// FOREVER_L pass; by value (volatile or not) it is read once after reset
void NN_axi(input_axi_t &in, output_axi_t out[BUFFER_SIZE], volatile bool &trigger, unsigned *window_size, unsigned *window_offset, unsigned *scaling_factor, unsigned *out_reset, unsigned *out_offset) {

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

        bool trig, reset;
        wait_trigger(trigger, out_reset, trig, reset);

        // Reset output buffer over AXI-lite / MMIO
        OUT_RESET_C: if (reset) {
            k = 0;
            *out_offset = 0;
        }

        // Trigger for readout data
        TRIGGER_C: if (trig) {

            bool load_done = false;
            bool buffer_ff_done = false;
            bool nn_done = false;

            load_done = load(in, in_local, *window_offset, scaling_factor);

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

