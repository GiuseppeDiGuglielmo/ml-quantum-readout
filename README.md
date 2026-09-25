# ml-quantum-readout

This project uses hls4ml to create the firmware implementation of machine learning algorithms for single and multi-qubit readout.

## Reference design (paper)

The design integrated in the QICK firmware and published in [End-to-End Workflow for Machine-Learning-Based Qubit Readout With QICK and hls4ml](https://ieeexplore.ieee.org/document/11159596) ([arXiv:2501.14663](https://arxiv.org/abs/2501.14663)) is the 800x4x1 two-layer MLP with ternary weights. Everything else in this repository is exploration around it.

| | |
|---|---|
| Input | 400 ADC samples x (I, Q) = 800 values, window starting at sample 100 |
| Layers | `fc1` QDense(4, ternary) -> `batchnorm1` -> `fc2` QDense(1, ternary, sigmoid) |
| Dataset | 20240528 single-qubit data (see [Data](#data)) |
| Board | ZCU216 RFSoC, Vivado 2020.1 |
| Paper results | 96% single-shot fidelity, 32 ns latency, 15.6% LUT overhead, 0 DSP |

How the pieces connect, from training to bitstream:

1. Training and hls4ml conversion: [notebooks/workflow_800x4x1_ternary.ipynb](notebooks/workflow_800x4x1_ternary.ipynb). The notebook is stored without outputs and writes to `checkpoints/scan_window_location_and_size_h4_ternary` and `hls4ml_projects/sl-100_ws-400_hn-4_vivado_ternary`, which are not in the repository.
2. HLS project: [hls_models/vivado_hls/two_layers_w400_ternary_h4_s100_20240528_hls4ml_prj](hls_models/vivado_hls/two_layers_w400_ternary_h4_s100_20240528_hls4ml_prj), with the trained model (`keras_model.keras`) and the hls4ml configuration (`hls4ml_config.yml`). Its exported IP is `NN_prj/solution1/impl/ip/xilinx_com_hls_NN_axi_1_0.zip` (not tracked in git).
3. Packaged IP, in the parent `qick_dev` repository: `qick_ml/ip/20240528/xilinx_com_hls_NN_axi_1_0_nonregistered_l2_w400_ternary_h4_s100.zip`. It is byte-identical to the IP exported in step 2. In the name, `l2` means two layers (not L2 regularization), `w400` the window size, `h4` the hidden neurons and `s100` the window start.
4. Vivado project, in the parent `qick_dev` repository: `firmware/proj_216_batch_nn.tcl` selects this IP with `MODEL_NAME "_l2"`, `WINDOW_SIZE 400`, `WINDOW_START 100` and `DATASET_DATE "20240528"`, and builds the ZCU216 bitstream.

## Setup

Clone repository

```bash
git clone https://github.com/GiuseppeDiGuglielmo/ml-quantum-readout.git
```

Create conda environment

```bash
conda env create -f environment.yml
conda activate ml4qick-env
```

## Data

Single qubit data can be found [here](https://purdue0-my.sharepoint.com/personal/du245_purdue_edu/_layouts/15/onedrive.aspx?id=%2Fpersonal%2Fdu245%5Fpurdue%5Fedu%2FDocuments%2FShared%2FQSC%20ML%20for%20readout%2FFinal%5Fraw%5Fdata%5Ffor%5Fpaper%2Fdata%5F0528%5Fnpy).
Other versions, and where the data lives in this repo, are described in [data/README.md](data/README.md).

## Training & Notebooks

Several notebooks for training exist, the simplest are the `workflow*.ipynb` notebooks. These notebooks start with training in (Q)Keras down to hls4ml IP generation. The scanning notebooks are useful for design space exploration, comparing traditional methods (match filtering and thresholding) with NNs of varying sizes.

The notebook for the paper design is `workflow_800x4x1_ternary.ipynb` (see [Reference design](#reference-design-paper)). `workflow_h4.ipynb` trains the same 800x4x1 shape with 4-bit weights instead of ternary, and `workflow.ipynb` and `workflow_h8.ipynb` train earlier, larger variants.

## Inference and Synthesis

The target device for the QICK system is the Zynq UltraScale+ [RFSoC ZCU216](https://www.xilinx.com/products/boards-and-kits/zcu216.html) Evaluation Kit. Synthesis results use Vivado 2020.1 (`setup.sh` sources it). The Vitis HLS projects in `hls_models/vitis_hls` target Vivado 2023.2.1.
<!-- 
```bash
cd inference 
python convert.py -c <framework>/<config>.yml
``` -->

## Useful Links

* [ZCU216 Evaluation Board User Guide](https://docs.xilinx.com/v/u/en-US/ug1390-zcu216-eval-bd)
* [ZCU216 PYNQ](https://github.com/sarafs1926/ZCU216-PYNQ)
