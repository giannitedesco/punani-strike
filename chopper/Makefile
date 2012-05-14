.SUFFIXES:

SPANK := ../spankassets
CONVERT := ../obj2asset.py
APACHE := ../data/apache.db
MISSILES := ../data/missiles.db
ROTOR := ../data/rotor.db
.PHONY: all clean

TARGET := all

all: $(APACHE) $(ROTOR) $(MISSILES)

$(APACHE): fuselage.g 
	$(SPANK) $@ $^

$(MISSILES): AGR_71_Hydra.g
	$(SPANK) $@ $^

$(ROTOR): rotor.g
	$(SPANK) $@ $^

%.g: %.obj %.mtl
	$(CONVERT) $<

clean:
	rm -f $(DB) $(patsubst %.obj, %.g, $(wildcard *.obj))
