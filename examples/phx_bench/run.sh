#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

cd $SCRIPT_DIR

limit=$((1024 * 1024 * 1024))

echo 'Size,Time(ns)' > phx.csv

for ((size=4096; $size <= $limit; size=$((2 * $size)))); do
	echo $size
	for i in {1..20}; do
		./snap $size >> phx.csv
		sleep 0.01
	done
done
#for i in {1..20}; do
#	./snap 4096 >> phx-snap.csv
#       sleep 0.01
#done
#for i in {1..20}; do
#	./snap 16777216 >> phx-snap.csv
#        sleep 0.01
#done
#for i in {1..20}; do
#	./snap 33554432 >> phx-snap.csv
#        sleep 0.01
#done
