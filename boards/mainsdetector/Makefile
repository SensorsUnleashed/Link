CONTIKI_PROJECT = mainsdetector

VERSION=0.0.1
DEVICE_TYPE=2

TARGETDIRS += ../../targets
CONTIKI=../../contiki-ng

# Include the CoAP implementation
MODULES += ../apps/sensorsunleashed ../apps/sensorsunleashed/resources 
MODULES += ../boards/dev
MODULES += os/net/app-layer/coap

all: $(CONTIKI_PROJECT)

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"
CFLAGS += -g -O0 -gdwarf-2

# linker optimizations
SMALL=1

CONTIKI_WITH_IPV6 = 1
include ../Makefile.ota
include $(CONTIKI)/Makefile.include
 
