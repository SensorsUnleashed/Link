CC       = arm-none-eabi-gcc
CPP      = arm-none-eabi-cpp
LD       = arm-none-eabi-gcc
AR       = arm-none-eabi-ar
OBJCOPY  = arm-none-eabi-objcopy
OBJDUMP  = arm-none-eabi-objdump
NM       = arm-none-eabi-nm
SIZE     = arm-none-eabi-size
SREC_CAT = srec_cat

SRCS+=bootloader.c
SRCS+=startup-gcc.c

CONTIKI=../contiki-ng
CONTIKI_CPU=$(CONTIKI)/arch/cpu/cc2538

SOURCEDIRS=. $(CONTIKI_CPU) $(CONTIKI_CPU)/dev $(CONTIKI)/os $(CONTIKI)/os/sys

#uart.c cc2538-rf.c int-master.c dbg.c
CONTIKI_CPU_SOURCEFILES += soc.c clock.c rtimer-arch.c  watchdog.c
CONTIKI_CPU_SOURCEFILES += nvic.c sys-ctrl.c gpio.c ioc.c spi.c adc.c
CONTIKI_CPU_SOURCEFILES += crypto.c aes.c ecb.c cbc.c ctr.c cbc-mac.c gcm.c
CONTIKI_CPU_SOURCEFILES += ccm.c sha256.c
CONTIKI_CPU_SOURCEFILES += cc2538-aes-128.c cc2538-ccm-star.c
CONTIKI_CPU_SOURCEFILES +=  udma.c lpm.c 
CONTIKI_CPU_SOURCEFILES += pka.c bignum-driver.c ecc-driver.c ecc-algorithm.c
CONTIKI_CPU_SOURCEFILES += ecc-curve.c
#CONTIKI_CPU_SOURCEFILES +=  ieee-addr.c
#CONTIKI_CPU_SOURCEFILES += slip-arch.c slip.c
CONTIKI_CPU_SOURCEFILES += i2c.c cc2538-temp-sensor.c vdd3-sensor.c
#CONTIKI_CPU_SOURCEFILES += cfs-coffee.c cfs-coffee-arch.c pwm.c

#include $(CONTIKI_CPU)/Makefile.cc2538

SRCS += $(CONTIKI_CPU_SOURCEFILES)

CFLAGS+=-I$(CONTIKI_CPU)
CFLAGS+=-I$(CONTIKI_CPU)/dev
CFLAGS+=-I$(CONTIKI)/arch/cpu/arm
CFLAGS+=-I$(CONTIKI)/arch/cpu/arm/cortex-m
CFLAGS+=-I$(CONTIKI)/arch/cpu/arm/cortex-m/cm3
CFLAGS+=-I$(CONTIKI)/arch/cpu/arm/cortex-m/CMSIS
CFLAGS+=-I$(CONTIKI)/os
CFLAGS+=-I$(CONTIKI)/os/sys

#Add the right prefix to all the 
vpath %.c $(SOURCEDIRS)
vpath %.S $(SOURCEDIRS)
CFLAGS += ${addprefix -I,$(SOURCEDIRS) $(CONTIKI)}

CFLAGS+=-mthumb -mcpu=cortex-m3
CFLAGS += -mthumb -mabi=aapcs -mlittle-endian
CFLAGS += -Werror -Wall
CFLAGS += -std=c99
CFLAGS += -ffunction-sections -fdata-sections -fno-strict-aliasing
CFLAGS += -fshort-enums -fomit-frame-pointer -fno-builtin
CFLAGS += -Os
CFLAGS+=-g -Wall -Werror -gdwarf-2

LDFLAGS += -mcpu=cortex-m3 -nostartfiles
LDFLAGS += -mthumb -mlittle-endian

OBJS=$(SRCS:.c=.o) $(ASRCS:.s79=.o)
DEPS=$(SRCS:.c=.d) $(ASRCS:.s79=.d)

OBJECTDIR = obj

# Linker options
LINKEROPTS=-mthumb
LINKEROPTS+=-mcpu=cortex-m3
LINKEROPTS+=-T cc2538-image.ld
LINKEROPTS+=-static
LINKEROPTS+=-Wl,-cref,-u,Reset_Handler
LINKEROPTS+=-Wl,--gc-sections

### This rule is used to generate the correct linker script
LDGENFLAGS += $(CFLAGS)
LDGENFLAGS += -imacros "contiki-conf.h" -imacros "dev/cc2538-dev.h"
LDGENFLAGS += -imacros "flash.h" -imacros "cfs-coffee-arch.h"
LDGENFLAGS += -x c -P -E

SOURCE_LDSCRIPT = cc2538.lds
#SOURCE_LDSCRIPT = cc2538.lds
#LDSCRIPT=cc2538-image.ld
LDSCRIPT = cc2538-image.ld



# The target application name.
BOOTLOADER=bootloader.elf

%.o: %.s79
	$(AS) $(ASFLAGS) -o $@ $<
	
all: $(LDSCRIPT) $(BOOTLOADER)
	$(OBJCOPY) -O binary $(BOOTLOADER) bootloader.odd
	#dd if=/dev/zero of=pad bs=1 count=`ls -l bootloader.odd | awk ' { pad = 0; size = $$5 ; rest = size % 16 ; if (rest > 0) { pad = 16 - rest;}  print pad }'` > /dev/null
	#cat bootloader.odd pad > bootloader
	#rm bootloader.odd pad
%.$(TARGET): %.elf %.hex %.bin
	$(Q)cp $< $@

# NB: Assumes LDSCRIPT was not overridden and is in $(OBJECTDIR)
$(LDSCRIPT): $(SOURCE_LDSCRIPT)
	$(TRACE_CC)
	$(Q)$(CC) $(LDGENFLAGS) $< | grep -v '^\s*#\s*pragma\>' > $@
	

$(BOOTLOADER): $(OBJS)
	$(CC) -o $@ $(LINKEROPTS) -Wl,-Map=$(@:.elf=.map) $(OBJS)
	$(SIZE) -A -x $@
	$(SIZE) -B -x $@
	
clean:
	rm -f $(BOOTLOADER) $(BOOTLOADER:.elf=.map) $(OBJS) $(OBJS:.o=.d)
	rm -f cc2538-image.ld bootloader cca.bin
