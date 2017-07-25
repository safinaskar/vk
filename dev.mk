#!/usr/bin/make -f

GEN_HEADER = grep '//@' $^ | sed 's~ *//@\( \|\)~~' > $@ || { rm -f $@; exit 1; }

all: libvk.hpp

dev-clean:
	rm -f libvk.hpp

libvk.hpp: libvk.cpp
	$(GEN_HEADER)
