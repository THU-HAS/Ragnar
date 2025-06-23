
V=( 0 64 128 173 192 256 )
A=( 0 16 32 48 64 80 96 112 128 144 160 176 192 208 224 240 256)
REMOTEUSER="[REMOTEUSER]"
REMOTEIP="[REMOTEIP]"
REMOTEBENCHMARK_DIR="[REMOTEBENCHMARK_DIR]"
REMOTECONFIG_DIR="[REMOTECONFIG_DIR]"
LOCALBENCHMARK_DIR="[LOCALBENCHMARK_DIR]"
LOCALCONFIG_DIR="[LOCALCONFIG_DIR]"
SSH_PORT=60022
for v in "${V[@]}"; do
    for a in {0..256..4}; do
        echo "mlx5_2 1 2097152 65536 16 1024 2 2" > "${LOCALCONFIG_DIR}/../sherman_config.txt"
        echo "READ 0 0 1 0_64_0 0_64_${a}" >> "${LOCALCONFIG_DIR}/../sherman_config.txt"
        echo "READ 1 1 1 0_64_0 0_64_${v}" >> "${LOCALCONFIG_DIR}/../sherman_config.txt"

        ssh "${REMOTEUSER}@${REMOTEIP}" -p "${SSH_PORT}" "killall benchmark"

        # ssh "${REMOTEUSER}@${REMOTEIP}" -p "${SSH_PORT}" "killall run_benchmark.sh"

        killall benchmark

        ssh "${REMOTEUSER}@${REMOTEIP}" -p "${SSH_PORT}" "bash ${REMOTEBENCHMARK_DIR}/restartMemc.sh" &
        
        sleep 3
        
        ssh "${REMOTEUSER}@${REMOTEIP}" -p "${SSH_PORT}" "${REMOTEBENCHMARK_DIR}/benchmark 2 0.5 1" &

        sleep 5

        "${LOCALBENCHMARK_DIR}/benchmark" 2 0.5 1 &

        sleep 20

        ssh "${REMOTEUSER}@${REMOTEIP}" -p "${SSH_PORT}" "killall benchmark"

        killall benchmark

    done
done

