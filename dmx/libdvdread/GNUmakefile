.PHONY: all clean depend cleandep install dvdread

all: dvdread

dvdread:
	@$(MAKE) -C src/ -f dvdread.mak
clean depend cleandep install:
	@$(MAKE) -C src/ -f dvdread.mak $@
