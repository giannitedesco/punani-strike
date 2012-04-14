#!/bin/sh

mkdir -p data/tiles data/map

unset SCALE

make -C assets && \
make -C chopper && \
make -C tiles

echo "SUCCESS"
