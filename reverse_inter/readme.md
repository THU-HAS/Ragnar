# Reverse Engineering (inter-traffic)

## Introduction

This is the reverse engineering part of inter-traffic.

## Prerequisites

`libibverbs` are required to run the RDMA traffics. If you use Nvidia/Mellanox RNICs, you can download and install `MLNX_OFED`.

## Usage

### a. Fillup the `config.json`

You should fill up the `config.json` file with your own configuration.

### b. Run the `new_test.py`

You can specify the configuration file with `--config` option and specify the output file with `--output` option.
