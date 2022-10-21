#! /bin/bash

make clean
sudo rm /mnt/pmem/po/poolfile
make

sudo ./rw_ll.o