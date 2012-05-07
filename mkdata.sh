#!/bin/sh

mkdir -p data/tiles data/maps data/font

unset SCALE

./mkfont.py  acknowtt.ttf carbon.ttf && \
cp conback.png splash.png smoke.png data/ && \
make -C assets && \
make -C chopper && \
make -C tiles && \
make -C maps && \
echo "SUCCESS"
