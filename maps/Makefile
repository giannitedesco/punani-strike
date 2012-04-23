.SUFFIXES:

MKMAP := ../mkmap

TARGET := all
.PHONY: all clean

all: $(patsubst %.m, ../data/maps/%, $(wildcard *.m))


../data/maps/%: %.m
	$(MKMAP) $@ $<

clean:
	rm -f $(patsubst %.m, ../data/maps/%, $(wildcard *.m))
