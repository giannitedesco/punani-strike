.SUFFIXES:

MKTILE := ../mktile

TARGET := all
.PHONY: all clean

all: $(patsubst %.t, ../data/tiles/%, $(wildcard *.t))


../data/tiles/%: %.t
	$(MKTILE) $@ $<

clean:
	rm -f $(patsubst %.t, ../data/tiles/%, $(wildcard *.t))
