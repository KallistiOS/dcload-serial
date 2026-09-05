CDRECORD  = wodim dev=0,0,0 speed=8
DD        = dd
CP        = cp
MKISOFS   = genisoimage
CAT       = cat
TOUCH     = touch

# Path to the ELF that mkdcdisc expects
LOADER_ELF = ../target-src/1st_read/loader.elf

# Default target
all: dctoolserial

# Build CDI using the ELF (not 1st_read.bin)
dctoolserial: $(LOADER_ELF)
	/opt/toolchains/dc/bin/mkdcdisc \
		-n $@ \
		-e $< \
		-N \
		-o $@.cdi \
		-v 3 \
		-m

# Build the ELF via the sub-project
$(LOADER_ELF):
	cd ../target-src && make

.PHONY: all clean

clean:
	rm -f dctoolserial.cdi
	cd ../target-src && make clean
