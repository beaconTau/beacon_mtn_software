##################################################
## Welcome to the beacon-ice-software Makefile ## 
##################################################

###################### Things you may need to change#################
# Location of libbeacon
LIBBEACON_DIR=../libbeacon

#installation prefix for programs 
PREFIX=/beacon

# location for config files 
BEACON_CONFIG_DIR=${PREFIX}/cfg 

####################################################################
# Things not meant to be changed 
####################################################################


CFLAGS +=-g -O2 -Iinclude -Wall -I$(LIBBEACON_DIR) -D_GNU_SOURCE
LDFLAGS+=-L$(LIBBEACON_DIR) -lbeacon -lflower8  -lz -lpthread -lconfig -lrt -lm

CC=gcc 
BUILDDIR=build
INCLUDEDIR=include
BINDIR=bin

.PHONY: clean install all doc default-configs

OBJS:= $(addprefix $(BUILDDIR)/, beacon-buf.o beacon-common.o beacon-cfg.o )
PROGRAMS := $(addprefix $(BINDIR)/, beacon-acq  beacon-copy \
																		beacon-make-default-config beacon-check-config \
																		beacon-set-saved-thresholds)
INCLUDES := $(addprefix $(INCLUDEDIR)/, $(shell ls $(INCLUDEDIR)))

all: $(PROGRAMS) etc/beacon.cfg 

etc/beacon.cfg: 
	mkdir -p etc 
	echo BEACON_PATH=${PREFIX}/bin > etc/beacon.cfg 
	echo BEACON_CONFIG_DIR=${BEACON_CONFIG_DIR}  >> etc/beacon.cfg 


default-configs: $(BINDIR)/beacon-make-default-config 
	$(BINDIR)/beacon-make-default-config acq cfg/acq.cfg
	$(BINDIR)/beacon-make-default-config copy cfg/copy.cfg



$(BUILDDIR): 
	mkdir -p $(BUILDDIR)

$(BINDIR): 
	mkdir -p $(BINDIR)

$(BUILDDIR)/%.o: src/%.c $(INCLUDES) Makefile | $(BUILDDIR)
	@echo Compiling  $< 
	@$(CC)  $(CFLAGS) -o $@ -c $< 

$(BINDIR)/%: src/%.c $(INCLUDES) $(OBJS) Makefile | $(BINDIR)
	@echo Compiling $<
	@$(CC) $(CFLAGS) $< $(OBJS) -o $@ -L./$(LIBDIR) $(LDFLAGS) 


install: $(PROGRAMS) $(INCLUDES) etc/beacon.cfg 
	install -d $(PREFIX)
	install -d $(PREFIX)/bin
	install $(PROGRAMS) $(PREFIX)/bin
	install etc/beacon.cfg /etc
	install -d $(PREFIX)/include
	install $(INCLUDES) $(PREFIX)/include 
	cp systemd/* /etc/systemd/system/
	cp scripts/* $(PREFIX)/bin
	systemctl daemon-reload
	#systemctl enable beacon-acq
	systemctl enable beacon-copy
	mkdir -p /data/daq

install-cfg:
	install -d $(PREFIX)/cfg
	cp cfg/* $(PREFIX)/cfg 


clean: 
	rm -rf build
	rm -rf bin
	rm -rf etc 






