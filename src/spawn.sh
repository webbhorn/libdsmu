#!/bin/sh

make clean; make;

./matrixmultiply 192.168.182.187 4444 1 2 >1.log &

./matrixmultiply 192.168.182.187 4444 2 2 >2.log &

