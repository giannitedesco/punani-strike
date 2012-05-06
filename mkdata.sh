#!/bin/sh

mkdir -p data/tiles data/maps

unset SCALE

cp splash.png smoke.png data/ && \
make -C assets && \
make -C chopper && \
make -C tiles && \
make -C maps

echo "SUCCESS"
