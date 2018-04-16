CONTIKI_PROJECT = debugdevice

VERSION=0.0.3
DEVICE_TYPE=1

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

#To create OTA images
# make otaprep TARGET=radioone IMG_TYPE=TOP
# make otaprep TARGET=radioone IMG_TYPE=BOT