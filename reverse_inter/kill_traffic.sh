#!/bin/bash
killall ib_write_bw 2>/dev/null
killall ib_read_bw 2>/dev/null
killall ib_atomic_bw 2>/dev/null
killall ib_send_bw 2>/dev/null
ssh abc "killall ib_write_bw 2>/dev/null"
ssh abc "killall ib_read_bw 2>/dev/null"
ssh abc "killall ib_atomic_bw 2>/dev/null"
ssh abc "killall ib_send_bw 2>/dev/null"