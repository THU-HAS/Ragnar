# Covert Channel (inter-traffic)

## Introduction

This is the covert channel part of inter-traffic.

## Prerequisites

`libibverbs` are required to run the RDMA traffics. If you use Nvidia/Mellanox RNICs, you can download and install `MLNX_OFED`.

You should do the following setup steps on both the server and the client side, to run the inter-traffic benchmarks.

- use switch_qos.sh to set up the traffic control rules.
- run richman.sh and then run thief.sh to start the covert channel.