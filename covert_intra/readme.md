# Covert Channel (intra-traffic)

## Introduction

This is the covert channel part of level-III/IV-granularity.

## Prerequisites

`libibverbs` are required to run the RDMA traffics. If you use Nvidia/Mellanox RNICs, you can download and install `MLNX_OFED`.

You should do the following setup steps on both the server and the client side, to run the intra-traffic benchmarks.

## Setup

1. Run `bash ./gen.sh` to generate the executable file `cc_test.o`
2. Write your own config file `config`. Otherwise you can modify and run `make_config_inter/intra_MR.sh` to generate a batch of config files in the folder `config_*`.

## Usage

### a. Run a single test with config file

1. Run `./cc_test.o server PORT CONFIGFILE` on the server side.
2. Run `./cc_test.o client SERVERADDR PORT CONFIGFILE` on the client side.
3. Run `killall -INT cc_test.o` on the server and the client side to end the test.
You should replace `PORT` with the same spare TCP port to exchange matadata, `SERVERADDR` with the server IP address, and `CONFIGFILE` with your config file.

### b. Run a batch of tests with config folder `config_*`

1. Modify remote and local addr, user, port, dir in `run_test.sh`.
2. Run `bash ./repeat_test.sh CONFIGFOLDER REPEAT_GROUPS`.
You should replace `CONFIGFOLDER` with your config folder name, and `REPEAT_GROUPS` with times you want to repeat the test.

## Format of config file

Config file is shown as below.
```
DEVICE_NAME IB_PORT MR_SIZE CQ_SIZE RQ_SIZE SQ_SIZE MR_NUM QP_NUM PERIOD
MSG_PATTERN_A1
MSG_PATTERN_A2
...
MSG_PATTERN_AN

MSG_PATTERN_B1
MSG_PATTERN_B2
...
MSG_PATTERN_BM

MSG_PATTERN_C1
MSG_PATTERN_C2
...
MSG_PATTERN_CK
```

Both side should use the same config file. (except for DEVICE_NAME and IB_PORT). `MSG_PATTERN_A*`s are the background (victim) traffic pattern. `MSG_PATTERN_B*`s are the covert traffic pattern of bit 0, and `MSG_PATTERN_C*`s are the covert traffic pattern of bit 1. `PERIOD` is the period of the covert channel.

`MSG_PATTERN` is shown as below.

```
OPCODE LOCAL_QP_IDX REMOTE_QP_IDX LOCAL_SGE_NUM SGE_1 ... LOCAL_SGE_N REMOTE_SGE
```

`LOCAL_SGE` and `REMOTE_SGE` are string like `a_b_c` where `a` is the index of MR, `b` is the message size, and `c` is the address offset.

## Result

The result are files for each corresponding config file. Each result file stores lines of `TIMESTAMP QPIDX LATENCY QUEUE_LENGTH`.

You can modify and run `draw.py` to process results from running `repeat_test.sh`.