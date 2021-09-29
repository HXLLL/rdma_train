#!/bin/bash

cd write_poll
make
cd ..

REMOTE_PATH=/home/huangxl/rdma_train

rsync -a --delete --exclude "*.o" ./ d:/home/huangxl/rdma_train

ssh d "screen -d -m bash -c \"cd $REMOTE_PATH/write_poll; make clean; make LD=aarch64-linux-gnu-gcc \" "
