.SUFFIXES:

SPANK := ../spankassets
CONVERT := ../obj2asset.py
DB := ../data/assets.db
.PHONY: all clean
PREFIX=assets

TARGET := all

all: $(DB)

$(DB): $(wildcard *.g) $(patsubst %.obj, %.g, $(wildcard *.obj))
	$(SPANK) $@ $(PREFIX)/ $^

%.g: %.obj
	SCALE=0.01 $(CONVERT) $<

clean:
	rm -f $(DB) $(patsubst %.obj, %.g, $(wildcard *.obj))
