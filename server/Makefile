CONTIKI_PROJECT = server dht22.h

all: $(CONTIKI_PROJECT)


# Do not try to build on Sky because of code size limitation
PLATFORMS_EXCLUDE = sky z1

# Include CoAP resources
MODULES_REL += ./resources
CONTIKI_TARGET_SOURCEFILES +=

CONTIKI=../../../../..

include ../Makefile.common

include $(CONTIKI)/Makefile.include




