# Covert channel (inter-traffic)

## Introduction

This is the side channel part of inter-traffic. We use disaggregated memory operations to set up the traffic, and we use traffic contention to set up the side channel.

## Prerequisites

`libibverbs` are required to run the RDMA traffics. If you use Nvidia/Mellanox RNICs, you can download and install `MLNX_OFED`.

The disaggregated memory operation code is in `M-RDMA-master`. You should download and compile it.

## 

After you set up the traffic, you can run the `shuffle_test.sh` to monitor the traffic contention.