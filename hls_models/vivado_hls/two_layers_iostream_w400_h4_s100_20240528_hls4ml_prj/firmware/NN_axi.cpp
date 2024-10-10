#include "NN_axi.h"

#include <ap_utils.h>

static bool load(
        input_qick_t &in,
        hls::stream<input_t> &in_local,
        unsigned scaling_factor
    ) {
    #pragma HLS INLINE off

    LOAD_OUTER_L: for(unsigned i = 0; i < N_IQ_WINDOW_IN*2 / input_t::size; ++i) {
        input_t ctype;
        #pragma HLS DATA_PACK variable=ctype

        LOAD_INNER_L: for(unsigned j = 0; j < input_t::size; ) {
            ap_uint<32> data_in;
            in.read(data_in);
            ctype[j++] = (typename input_t::value_type(data_in.range(15,0))) * scaling_factor;
            ctype[j++] = (typename input_t::value_type(data_in.range(31,16))) * scaling_factor;
        }
        in_local.write(ctype);
    }

    return true;
}

#ifdef __SYNTHESIS__
static void reset_local_buffers(
        hls::stream<input_t> &in_local,
        hls::stream<result_t> &out_local,
        unsigned *out_reset,
        unsigned *out_offset,
        unsigned *k
    ) {
    OUT_RESET_C: if ((*out_reset) == 255) {
        *k = 0;
        *out_offset = 0;
// TODO: This is not necessary with streaming
//        RESET_IN_LOCAL_L: for (unsigned i = 0; i < N_IQ_WINDOW_IN*2; i++) {
//            #pragma HLS UNROLL
//            in_local.write(0);
//        }
//        RESET_OUT_LOCAL_L: for (unsigned i = 0; i < N_OUT; i++) {
//            #pragma HLS UNROLL
//            out_local.push(0);
//        }
//        RESET_OUT_L: for (unsigned i = 0; i < BUFFER_SIZE; i++) {
//            #pragma HLS UNROLL
//            out[i] = 0;
//        }
    }
}
#endif

static void store(
        hls::stream<result_t> &out_local,
        int offset,
        output_qick_t out[BUFFER_SIZE]
    ) {
    #pragma HLS INLINE off

    result_t out_ctype = out_local.read();
    //#pragma HLS DATA_PACK variable=out_ctype
    STORE_L: for(unsigned i = 0; i < N_OUT; i++) {
        out[offset + i] = out_ctype[i].to_float();
    }
}

void NN_axi(
        input_qick_t &in,
        output_qick_t out[BUFFER_SIZE],
        bool trigger,
        unsigned *window_size,
        unsigned *window_offset,
        unsigned *scaling_factor,
        unsigned *out_reset,
        unsigned *out_offset
        ) {

    // Unregistered axis
    //#pragma HLS INTERFACE axis off port=in
    // Registered axis
    #pragma HLS INTERFACE axis register both port=in

    #pragma HLS INTERFACE bram depth=4294967295 latency=1 port=out
    #pragma HLS INTERFACE ap_none port=trigger
    #pragma HLS INTERFACE s_axilite register port=window_size bundle=config
    #pragma HLS INTERFACE s_axilite register port=window_offset bundle=config
    #pragma HLS INTERFACE s_axilite register port=scaling_factor bundle=config
    #pragma HLS INTERFACE s_axilite register port=out_reset bundle=config
    #pragma HLS INTERFACE s_axilite register port=out_offset bundle=config
    #pragma HLS INTERFACE ap_ctrl_none port=return

    // I/O buffers of the hls4ml NN module
    hls::stream<input_t> in_local("input_1");
    hls::stream<result_t> out_local("output_1");

    #pragma HLS STREAM variable=in_local depth=1
    #pragma HLS STREAM variable=out_local depth=1

    bool load_done = false;
    bool nn_done = false;


    // Index of the output buffer over AXI-lite / MMIO
    unsigned k = 0;

    // Always active
#ifdef __SYNTHESIS__
    FOREVER_L: do {

        // Reset output buffer over AXI-lite / MMIO
        reset_local_buffers(in_local, out_local, out_reset, out_offset, &k);

        // Trigger for readout data
        TRIGGER_C: if (trigger) {

            // If you need you can wait extra clock cycles
            WINDOW_OFFSET_L: ap_wait_n(*window_offset);
#endif

            // Read readout data
            load_done = load(in, in_local, *scaling_factor);

            // hls4ml NN module
            nn_done = NN(in_local, out_local);

            // Output logits (ground [0], excited [1])
            if (load_done && nn_done) {
                store(out_local, k*N_OUT, out);
            }

            // Increment and reset index of the output buffer over AXI-lite / MMIO
            k++;
            if (k*N_OUT >= BUFFER_SIZE)
                k = 0;

            // Keep track of the current index via AXI-lite / MMIO
            *out_offset = k;
#ifdef __SYNTHESIS__
        }
    } while (true);
#endif
}



