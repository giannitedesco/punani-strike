.SUFFIXES:

SPANK := ../spankassets
CONVERT := ../obj2asset.py
DB := ../data/assets.db
.PHONY: all clean

TARGET := all

all: $(DB)

$(DB): $(wildcard *.g) $(patsubst %.obj, %.g, $(wildcard *.obj))
	$(SPANK) $@ $^

%.g: %.obj %.mtl
	$(CONVERT) $<

clean:
	rm -f $(DB) $(patsubst %.obj, %.g, $(wildcard *.obj))
