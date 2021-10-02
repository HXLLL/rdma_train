#!/bin/bash

cd write_poll
make
cd ..

DPU_HOSTNAME=dpu
REMOTE_PATH=/home/ubuntu/rdma_train

rsync -a --delete --exclude "*.o" ./ $DPU_HOSTNAME:$REMOTE_PATH

ssh $DPU_HOSTNAME "screen -d -m bash -c \"cd $REMOTE_PATH/write_poll; make clean; make LD=aarch64-linux-gnu-gcc \" "
