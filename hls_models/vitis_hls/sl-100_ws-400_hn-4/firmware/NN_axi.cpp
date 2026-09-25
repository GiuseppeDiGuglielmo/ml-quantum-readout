#include "NN_axi.h"

static void load(
		const input_axi_t *in,
		input_t (&in_buf)[N_IN]
) {
	LOAD_L: for (int i = 0; i < N_IN; i++) {
		#pragma HLS PIPELINE II=1
		input_axi_t tmp = in[i];
		in_buf[i] = tmp.data;
    }
}

static void compute(
		input_t (&in_buf)[N_IN],
		result_t (&out_buf)[N_OUT]
) {
	NN(in_buf, out_buf);
}

static void store(
		output_axi_t *out,
		result_t (&out_buf)[N_OUT]
) {
	STORE_L: for (int i = 0; i < N_OUT; i++) {
		#pragma HLS PIPELINE II=1
        out[i] = out_buf[i].to_float();
    }
}



void NN_axi(input_axi_t in[N_IN], output_axi_t out[N_OUT]) {

    #pragma HLS INTERFACE axis port=in
    #pragma HLS INTERFACE axis port=out
    #pragma HLS INTERFACE ap_ctrl_none port=return

	#pragma HLS DATAFLOW

    input_t in_local[N_IN];
    result_t out_local[N_OUT];

	#pragma HLS STREAM depth=1 variable=in_local
	#pragma HLS STREAM depth=1 variable=out_local
	//#pragma HLS ARRAY_RESHAPE dim=1 type=complete variable=in_local
	//#pragma HLS ARRAY_RESHAPE dim=1 type=complete variable=out_local

    load(in, in_local);

    compute(in_local, out_local);

    store(out, out_local);

}
