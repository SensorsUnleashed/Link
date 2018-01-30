CONTIKI_PROJECT = er-uart-server
PROJECT_SOURCEFILES += 	susensors.c pairing.c pulsesensor.c \
						relay.c susensorcommon.c ledindicator.c \
						mainsdetect.c button-sensor.c cmp.c cmp_helpers.c \
						reverseNotify.c mmem.c

all: $(CONTIKI_PROJECT)

TARGETDIRS += ../targets

APPDIRS += ../apps

CONTIKI=../contiki-ng

# Include the CoAP implementation
MODULES += os/net/app-layer/coap

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

ifeq ($(TARGET), native)
CFLAGS += -DNATIVE=1
PROJECT_SOURCEFILES += coap_proxy_test.c
endif

ifeq ($(TARGET), minimal-net)
CFLAGS += -DNATIVE=1
PROJECT_SOURCEFILES += coap_proxy_test.c
endif 

ifeq ($(TARGET), su-cc2538-1)
CFLAGS += -DNATIVE=0
endif

ifeq ($(TARGET), radioone)
CFLAGS += -DNATIVE=0
endif

# automatically build RESTful resources
REST_RESOURCES_DIR = ./resources
ifndef TARGET
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c'))
else
ifeq ($(TARGET), native)
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c'))
else
REST_RESOURCES_FILES = $(notdir $(shell find $(REST_RESOURCES_DIR) -name '*.c' ! -name 'res-plugtest*'))
endif
endif

PROJECTDIRS += $(REST_RESOURCES_DIR) ./dev ./lib
PROJECT_SOURCEFILES += $(REST_RESOURCES_FILES)

# linker optimizations
SMALL=1

# REST Engine shall use Erbium CoAP implementation
#APPS += uartsensors
APPS += er-coap
APPS += rest-engine

# optional rules to get assembly
#CUSTOM_RULE_C_TO_OBJECTDIR_O = 1
#CUSTOM_RULE_S_TO_OBJECTDIR_O = 1

CONTIKI_WITH_IPV6 = 1
include $(CONTIKI)/Makefile.include

# minimal-net target is currently broken in Contiki
ifeq ($(TARGET), minimal-net)
CFLAGS += -DHARD_CODED_ADDRESS=\"fd81:3daa:fb4a:f7ae\" 
${info INFO: er-example compiling with large buffers}
CFLAGS += -DUIP_CONF_BUFFER_SIZE=1300
CFLAGS += -DREST_MAX_CHUNK_SIZE=1024
CFLAGS += -DCOAP_MAX_HEADER_SIZE=176
CFLAGS += -DUIP_CONF_RECEIVE_WINDOW=1000

CONTIKI_WITH_RPL=0



#CFLAGS += -DUIP_CONF_ND6_DEF_MAXDADNS=0
endif

#connect-minimal:
#	sudo ip address add fdfd::1/64 dev tap0
#  	#sudo ip address add fd81:3daa:fb4a:f7ae:a537:bb41:56f9:3020 dev tap0
#  	sudo ip address add fe80::dead:beef/64 dev tap0
  
  
