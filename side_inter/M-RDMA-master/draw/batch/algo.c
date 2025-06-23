// SP
for buffer in buffers:
    memcpy(buffer, tmp_buffer)
wr = gen_rdma_wr(&tmp_buffer)

// SGL
for buffer in buffer:
    sgl.push(buffer)
wr = gen_rdma_wr(sgl)
rdma_write(wr)

// Doorbell
for buffer in buffer:
    wr = gen_rdma_wr(buffer)
    wr_list.push(wr)
rdma_write(wr_list)



