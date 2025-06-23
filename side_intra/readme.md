# Side Channel (intra-traffic)

## Introduction

This is the side channel part of level-III/IV-granularity. We have done several modifications on `Sherman` to run with latest version of `MLNX_OFED`, to emulate file-accessing-like behavior, and to set up side channels.

## Prerequisites

`libibverbs` are required to run the RDMA traffics. If you use Nvidia/Mellanox RNICs, you can download and install `MLNX_OFED`.

Prerequisites of `Sherman` are also needed to be satisfied (see https://github.com/thustorage/Sherman).

You should do the following setup steps on both the server and the client side, to run the intra-traffic benchmarks.

## Setup

1. Build the modified `Sherman` following steps in https://github.com/thustorage/Sherman.
2. Modify and run `side_channel/Sherman_test.sh`.

## Results

1. Modify and run `side_channel/draw.py` to generate patterns of variant victim offsets.