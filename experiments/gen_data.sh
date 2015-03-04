#!/bin/sh
#
# gen_data.sh

rec_len=56
dataset_size=$(( 8 * 1024 * 1024 * 1024 ))
nrecs=$(( $dataset_size / (8 + $rec_len) ))

# uniform
dd if=/dev/zero of=data.out bs=1M count=1024

# uniform random
dd if=/dev/urandom of=data.out bs=1M count=1024

# normal distribution
mean="0.0"
stddev="1.0"
gen_dist_data -n $nrecs $rec_len $mean $stddev

# poisson distribution
lambda="1.0"
gen_dist_data -p $nrecs $rec_len $lambda

