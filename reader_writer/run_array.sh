#! /bin/bash

make clean
sudo rm /mnt/pmem/po/pool*.obj
make

sudo ./read_write.o