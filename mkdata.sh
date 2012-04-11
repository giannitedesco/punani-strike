#!/bin/sh

./spankassets data/assets.db assets/* && \
./mktile data/tiles/city00 tiles/city00.t && \
./mktile data/tiles/null tiles/null.t && \

./obj2asset.py chopper/*.obj && \
./spankassets data/choppers.db chopper/*.g && \
echo "SUCCESS"
