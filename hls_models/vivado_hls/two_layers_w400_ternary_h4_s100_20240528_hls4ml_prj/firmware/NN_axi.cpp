#include <gmp.h>
#define __gmp_const const

#include "NN_axi.h"

#include <ap_utils.h>
#include <utils/x_hls_utils.h>

// scaling_factor is an input gain: each 16-bit I and Q sample is multiplied
// by it before it enters the window, so a signal recorded at 1/k scale
// reaches the NN at the scale it was trained on. The gain is a power of two,
// 1, 2, 4 or 8, applied as a left shift, from the low 4 bits of the
// register: other values round down to one of these (9-15 to 8, and only
// bits 3:0 count, so 16 means 1), and 0, the value after reset, means 1.
// The NN takes the low 14 bits of the result, as it always took bits 13:0 of
// each sample: with gain 1 the IP behaves exactly as before, and a sample
// whose product does not fit 14 bits wraps around. Saturating instead would need a
// compare and a mux after it, more than one cycle's budget, and LOAD_L has to
// take one sample per cycle (the readout stream does not wait).

// Priority encoder of bits 3:0, written as bit logic (no compares, no branches):
// it sits right after the trigger, where every extra delay could add a cycle
static ap_uint<2> gain_shift(ap_uint<4> sf) {
    #pragma HLS INLINE
    ap_uint<2> sh;
    sh[1] = sf[3] | sf[2];
    sh[0] = sf[3] | (~sf[2] & sf[1]);
    return sh;
}

// Reads offset + N_IQ_WINDOW_IN samples, one per cycle; the shift register
// keeps the last N_IQ_WINDOW_IN, so the window starts offset samples after
// the first read (window_offset counts samples). LOAD_L is not pipelined
// (see below): one sample per cycle only holds while its whole body fits in
// one cycle, so keep it free of multipliers, compares and branches
bool load(input_axi_t &in, input_t in_local[N_IQ_WINDOW_IN*2], unsigned offset, ap_uint<2> shift) {
    #pragma HLS INLINE
    // Read readout data
    // No rewind: it made HLS drop the pipeline (NN() is a dataflow region
    // in the same FOREVER_L body), so LOAD_L ran at II=3
    LOAD_L: for(unsigned i = 0; i < offset + N_IQ_WINDOW_IN; i++) {
        #pragma HLS PIPELINE II=1
        #pragma HLS LOOP_TRIPCOUNT min=400 max=770
        ap_uint<32> data_in;
        in.read(data_in);
        // I (bits 13:0) and Q (bits 29:16), shifted by the gain, low 14 bits
        ap_int<14> i14 = data_in.range(13,0);
        ap_int<14> q14 = data_in.range(29,16);
        input_t lo = (ap_int<14>) (i14 << shift);
        input_t hi = (ap_int<14>) (q14 << shift);
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
// FOREVER_L, because FOREVER_L also holds the NN dataflow region.
// scaling_factor is re-read on every pass too, so a gain written while the
// IP waits applies to the next window, and decoding it adds nothing between
// the trigger and the first sample read
void wait_trigger(volatile bool &trigger, volatile unsigned *out_reset, volatile unsigned *scaling_factor,
                  bool &trig, bool &reset, ap_uint<4> &gain_bits) {
    #pragma HLS INLINE off
    bool t, r;
    ap_uint<4> g;
    TRIG_WAIT_L: do {
        #pragma HLS PIPELINE II=1
        t = trigger;
        r = (*out_reset == 255);
        g = *scaling_factor;
    } while (!t && !r);
    trig = t;
    reset = r;
    gain_bits = g;
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
        ap_uint<4> gain_bits;
        wait_trigger(trigger, out_reset, scaling_factor, trig, reset, gain_bits);
        ap_uint<2> shift = gain_shift(gain_bits);

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

            load_done = load(in, in_local, *window_offset, shift);

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

