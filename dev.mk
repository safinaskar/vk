#!/usr/bin/make -f

GEN_HEADER = grep '//@' $^ | sed 's~ *//@\( \|\)~~' > $@ || { rm -f $@; exit 1; }

all: libvk.h

dev-clean:
	rm -f libvk.h

libvk.h: libvk.cpp
	$(GEN_HEADER)
