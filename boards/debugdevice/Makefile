CONTIKI_PROJECT = debugdevice

TARGETDIRS += ../../targets
CONTIKI=../../contiki-ng

# Include the CoAP implementation
MODULES += ../apps/sensorsunleashed ../apps/sensorsunleashed/resources 
MODULES += ../boards/dev
MODULES += os/net/app-layer/coap

all: $(CONTIKI_PROJECT)

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

# linker optimizations
SMALL=1

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

 
  
