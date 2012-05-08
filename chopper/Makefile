.SUFFIXES:

SPANK := ../spankassets
CONVERT := ../obj2asset.py
APACHE := ../data/apache.db
ROTOR := ../data/rotor.db
.PHONY: all clean

TARGET := all

all: $(APACHE) $(ROTOR)

$(APACHE): fuselage.g  AGR_71_Hydra.g
	$(SPANK) $@ $^

$(ROTOR): rotor.g
	$(SPANK) $@ $^

%.g: %.obj
	$(CONVERT) $<

clean:
	rm -f $(DB) $(patsubst %.obj, %.g, $(wildcard *.obj))
