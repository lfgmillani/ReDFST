.PHONY: all bin lib clean clean-bin clean-lib install install-bin install-lib
all: bin lib
install: install-bin install-lib
clean: clean-bin clean-lib

install-bin: bin
	+ make -C redfst install

install-lib: lib
	+ make -C libredfst install

bin:
	+ make -C redfst

lib:
	+ make -C libredfst

clean-bin:
	+ make -C redfst clean

clean-lib:
	+ make -C libredfst clean
