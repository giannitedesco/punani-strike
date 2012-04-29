.SUFFIXES:

SPANK := ../spankassets
CONVERT := ../obj2asset.py
COMANCHE := ../data/comanche.db
ROTOR := ../data/rotor.db
.PHONY: all clean

TARGET := all

all: $(COMANCHE) $(ROTOR)

$(COMANCHE): fuselage_black.g  fuselage_green.g AGR_71_Hydra.g
	$(SPANK) $@ $^

$(ROTOR): rotor.g
	$(SPANK) $@ $^

%.g: %.obj
	$(CONVERT) $<

clean:
	rm -f $(DB) $(patsubst %.obj, %.g, $(wildcard *.obj))
