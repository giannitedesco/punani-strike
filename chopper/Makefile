.SUFFIXES:

SPANK := ../spankassets
CONVERT := ../obj2asset.py
DB := ../data/choppers.db
.PHONY: all clean

TARGET := all

all: $(DB)

$(DB): $(wildcard *.g) $(patsubst %.obj, %.g, $(wildcard *.obj))
	$(SPANK) $@ $^

%.g: %.obj
	SCALE=1.0 $(CONVERT) $<

clean:
	rm -f $(DB) $(patsubst %.obj, %.g, $(wildcard *.obj))
