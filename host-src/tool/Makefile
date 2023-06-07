include ../../Makefile.cfg

LZOPATH = ../../minilzo.106

CC		= $(HOSTCC)
CFLAGS	= $(HOSTCFLAGS) -DDCLOAD_VERSION=\"$(VERSION)\" -DDEFAULT_SPEED=$(TOOL_DEFAULT_SPEED) -DSERIALDEVICE="\"$(SERIALDEVICE)\"" -DHAVE_GETOPT -DB1500000 -DB500000
LDFLAGS = $(HOSTLDFLAGS)
INCLUDE = -I$(LZOPATH) -I/usr/local/include

# Adding static flag if asked
ifeq ($(STANDALONE_BINARY),1)
  LDFLAGS += -static
endif

# Determine if we need zlib or not in that context for building dc-tool
# The purpose of that is just to have '-lz' once at the end of the command line
ZLIB_REQUIRED := 0

# Linking with 'libelf' or 'libbfd' (sh-elf), depending of 'Makefile.cfg'
ifeq ($(WITH_BFD),1)
  # Starting from Binutils 2.40, SFrame is required when using BFD
  LIBSFRAME	=
  ifneq ("$(wildcard $(TARGETPREFIX)/lib/libsframe.a)","")
    LIBSFRAME = -lsframe
  endif
  
  CFLAGS	+= -DWITH_BFD
  LDFLAGS	+= -L$(BFDLIB) -lbfd $(LIBSFRAME) -liberty -lintl
  INCLUDE	+= -I$(BFDINCLUDE)
  
  ZLIB_REQUIRED := 1
else
  LDFLAGS	+= -L$(ELFLIB) -lelf
  INCLUDE	+= -I$(ELFINCLUDE)
endif

# Additional libraries for MinGW/MSYS or MinGW-w64/MSYS2
ifdef MINGW32
  LDFLAGS	+= -lws2_32 -lwsock32 -liconv
  ZLIB_REQUIRED := 1
endif

# Add zlib to the command line end... if required
ifeq ($(ZLIB_REQUIRED),1)
  LDFLAGS	+= -lz
endif

DCTOOL	= dc-tool-ser$(EXECUTABLEEXTENSION)

OBJECTS	= dc-tool.o minilzo.o syscalls.o unlink.o utils.o
LZOFILES = $(LZOPATH)/minilzo.c $(LZOPATH)/minilzo.h $(LZOPATH)/lzoconf.h

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $<

all: $(DCTOOL)

$(DCTOOL): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

minilzo.o: $(LZOFILES)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ -c $< 

.PHONY : install
install: $(DCTOOL) | $(TOOLINSTALLDIR)
	cp $(DCTOOL) $(TOOLINSTALLDIR)
$(TOOLINSTALLDIR):
	-mkdir $(TOOLINSTALLDIR)

.PHONY : clean
clean:
	rm -f $(OBJECTS)

.PHONY : distclean
distclean: clean
	rm -f $(DCTOOL)
