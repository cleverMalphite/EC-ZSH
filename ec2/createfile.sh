#!/bin/bash


#dd if=/dev/zero of=largefile bs=1G count=1 # create 1G 0 data file

dd if=/dev/urandom of=../../FileSend/test800MB.txt bs=1M count=800 # create 10MB random data file
