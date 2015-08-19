# multicat Makefile
JSON_C_DIR=/usr/local
BITSTREAM_C_DIR=/usr/local

VERSION = 2.0
CFLAGS += -Wall -Wformat-security -O3 -fomit-frame-pointer -D_FILE_OFFSET_BITS=64 -D_ISOC99_SOURCE -D_BSD_SOURCE
CFLAGS += -g
# Comment out the following line for Mac OS X build
LDLIBS += -lrt -lpthread

CFLAGS += -I$(JSON_C_DIR)/include/json-c
CFLAGS += -I$(BITSTREAM_C_DIR)/include/bitstream
LDFLAGS_SUPERVISOR = $(LDLIBS) -L$(JSON_C_DIR)/lib -ljson-c -lmysqlclient -lz
LDFLAGS_VIEWER = $(LDLIBS)

OBJ_MULTICAT = multicat.o util.o eit.o lib_ini.o sharedMemoryLib.o logs.o
OBJ_MULTICAT_VIEWER = multicat-viewer.o util.o lib_ini.o sharedMemoryLib.o logs.o
OBJ_MULTICAT_SUPERVISOR = multicat-supervisor.o util.o lib_ini.o sharedMemoryLib.o eit_mysql.o json_parsing.o logs.o
OBJ_INGESTS = ingests.o util.o lib_ini.o logs.o
OBJ_AGGREGARTP = aggregartp.o util.o lib_ini.o logs.o
OBJ_REORDERTP = reordertp.o util.o lib_ini.o logs.o
OBJ_OFFSETS = offsets.o util.o lib_ini.o logs.o
OBJ_LASTS = lasts.o
OBJ_MULTICAT_VALIDATE = multicat_validate.o util.o lib_ini.o logs.o

PREFIX ?= /usr/local/multicat-tools
BIN = $(DESTDIR)/$(PREFIX)/bin
MAN = $(DESTDIR)/$(PREFIX)/share/man/man1
CONF = $(PREFIX)/conf
SHPATH = $(PREFIX)/sh

all: multicat-eit multicat-supervisor multicat-viewer ingests aggregartp reordertp offsets lasts multicat_validate

$(OBJ_MULTICAT): Makefile util.h eit.h lib_ini.h sharedMemoryLib.h logs.h
$(OBJ_MULTICAT_VIEWER) : Makefile lib_ini.h sharedMemoryLib.h logs.h
$(OBJ_MULTICAT_SUPERVISOR) : Makefile util.h lib_ini.h sharedMemoryLib.h eit_mysql.h json_parsing.h logs.h
$(OBJ_INGESTS): Makefile util.h
$(OBJ_AGGREGARTP): Makefile util.h
$(OBJ_REORDERTP): Makefile util.h
$(OBJ_OFFSETS): Makefile util.h
$(OBJ_LASTS): Makefile
$(OBJ_MULTICAT_VALIDATE): Makefile util.h

multicat-eit: $(OBJ_MULTICAT)
	$(CC) -o $@ $(OBJ_MULTICAT) $(LDLIBS)

multicat-viewer: $(OBJ_MULTICAT_VIEWER)
	$(CC) -o $@ $(OBJ_MULTICAT_VIEWER) $(LDFLAGS_VIEWER)

multicat-supervisor: $(OBJ_MULTICAT_SUPERVISOR)
	$(CC) -o $@ $(OBJ_MULTICAT_SUPERVISOR) $(LDFLAGS_SUPERVISOR)

ingests: $(OBJ_INGESTS)
	$(CC) -o $@ $(OBJ_INGESTS) $(LDLIBS)

aggregartp: $(OBJ_AGGREGARTP)
	$(CC) -o $@ $(OBJ_AGGREGARTP) $(LDLIBS)

reordertp: $(OBJ_REORDERTP)
	$(CC) -o $@ $(OBJ_REORDERTP) $(LDLIBS)

offsets: $(OBJ_OFFSETS)
	$(CC) -o $@ $(OBJ_OFFSETS) $(LDLIBS)

lasts: $(OBJ_LASTS)
	$(CC) -o $@ $(OBJ_LASTS) $(LDLIBS)

multicat_validate: $(OBJ_MULTICAT_VALIDATE)
	$(CC) -o $@ $(OBJ_MULTICAT_VALIDATE) $(LDLIBS)

clean:
	-rm -f multicat-eit $(OBJ_MULTICAT) multicat-viewer $(OBJ_MULTICAT_VIEWER) multicat-supervisor $(OBJ_MULTICAT_SUPERVISOR) ingests $(OBJ_INGESTS) aggregartp $(OBJ_AGGREGARTP) reordertp $(OBJ_REORDERTP) offsets $(OBJ_OFFSETS) lasts $(OBJ_LASTS) multicat_validate

install: all
	@install -d $(BIN)
	@install -d $(MAN)
	@install -d $(CONF)
	@install -d $(SHPATH)

	@install multicat-eit multicat-supervisor multicat-viewer ingests aggregartp reordertp offsets lasts multicat_validate $(BIN)
	@install multicat.1 ingests.1 aggregartp.1 reordertp.1 offsets.1 lasts.1 $(MAN)
	@install multicat.ini multicat_viewer.ini $(CONF)
	@install multicat_expire.sh multicat.sh multicat-supervisor.sh $(SHPATH)

uninstall:
	@rm $(BIN)/multicat-eit $(BIN)/multicat-supervisor $(BIN)/multicat-viewer $(BIN)/ingests $(BIN)/aggregartp $(BIN)/reordertp $(BIN)/offsets $(BIN)/lasts $(BIN)/multicat_validate
	@rm $(MAN)/multicat.1 $(MAN)/ingests.1 $(MAN)/aggregartp.1 $(MAN)/reordertp.1 $(MAN)/offsets.1 $(MAN)/lasts.1


