#!/bin/bash
# ALL SPECIFIC FOR CU CS VM FALL 2017

sudo mknod -m 777 /dev/simple_character_device c 240 0
sudo rmmod simpleCharDriver
make -C /lib/modules/4.10.0-35-generic/build M=$PWD modules
sudo insmod simpleCharDriver.ko
