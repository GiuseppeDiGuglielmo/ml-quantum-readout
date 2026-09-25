#include "NN_axi.h"

#include <ap_utils.h>

// Results on AXI-lite
//void NN_axi(input_axi_t &in, bool trigger, unsigned *scaler, unsigned *trigger_delay, output_t *g_logit, output_t *e_logit) {
//	#pragma HLS INTERFACE s_axilite port=e_logit bundle=config
//	#pragma HLS INTERFACE s_axilite port=g_logit bundle=config


void NN_axi(
        hls::stream<input_axi_t> &in_stream,
        output_axi_t out[N_OUT],
        bool trigger,
        unsigned *scaler,
        unsigned *trigger_delay) {
    //#pragma HLS LATENCY min=1
    #pragma HLS INTERFACE axis port=in_stream
    #pragma HLS INTERFACE bram depth=4294967295 latency=1 port=out

    // Registered axis
    //#pragma HLS INTERFACE axis register both port=in
    // Unregistered axis
	//#pragma HLS INTERFACE axis off port=in

    #pragma HLS INTERFACE ap_none port=trigger
	#pragma HLS INTERFACE s_axilite port=scaler bundle=config
	#pragma HLS INTERFACE s_axilite port=trigger_delay bundle=config
    #pragma HLS INTERFACE ap_ctrl_none port=return

    hls::stream<input_t> in_local_stream;
    hls::stream<result_t> out_local_stream;

    input_t in_local;
    result_t out_local;

    //unsigned trigger_delay_local = *trigger_delay;
    //unsigned scaler_local = *scaler;

    //FOREVER_L: do {
        //TRIGGER_C: if (trigger) {
            //TRIGGER_DELAY_L: ap_wait_n(*trigger_delay);

            LOAD_L: for(unsigned i = 0, j = 0; i < N_IQ_IN; i++) {
                #pragma HLS PIPELINE
            	ap_uint<32> data_in;
            	in_stream.read(data_in);
//            	DATA_C: if (i >= N_IQ_OFFSET_IN && i <= N_IQ_OFFSET_IN+N_IQ_WINDOW_IN) {
                    ap_fixed<14,14> lo = data_in.range(15,0) * *scaler;
                    ap_fixed<14,14> hi = data_in.range(31,16) * *scaler;
            		in_local[j++] = lo;
            		in_local[j++] = hi;
//            	}
            }

            in_local_stream.write(in_local);

        	NN(in_local_stream, out_local_stream);

            out_local_stream.read(out_local);

        	STORE_L: for(unsigned i = 0; i < N_OUT; i++){
            	#pragma HLS PIPELINE
            	out[i] = out_local[i];
        	}

            // Results on AXI-lite
            //*g_logit = out_local[0];
            //*e_logit = out_local[1];
        //}
    //} while (true);
}



