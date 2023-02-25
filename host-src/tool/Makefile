include ../../Makefile.cfg

LZOPATH = ../../minilzo.106

CC		= $(HOSTCC)
CFLAGS	= $(HOSTCFLAGS) -DDCLOAD_VERSION=\"$(VERSION)\" -DBAUD_RATE=$(SERIALSPEED) -DSERIALDEVICE="\"$(SERIALDEVICE)\"" -DHAVE_GETOPT -DB1500000 -DB500000
LDFLAGS = $(HOSTLDFLAGS)
INCLUDE	= -I$(LZOPATH) -I/usr/local/include

ifeq ($(WITH_BFD),1)
  CFLAGS	+= -DWITH_BFD
  LDFLAGS	= -L$(BFDLIB) -lbfd -liberty -lintl -lz
  INCLUDE	+= -I$(BFDINCLUDE)
else
  LDFLAGS	= -L$(ELFLIB) -lelf
  INCLUDE	+= -I$(ELFINCLUDE)
endif

# Additional libraries for MinGW/MSYS
ifdef MINGW32
  LDFLAGS	+= -lwsock32 -liconv
endif

ifdef STANDALONE_BINARY
  LDFLAGS 	+= -static
endif

DCTOOL	= dc-tool-ser$(EXECUTABLEEXTENSION)

OBJECTS	= dc-tool.o minilzo.o syscalls.o unlink.o
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
