C++ = g++

ifndef os
   os = LINUX
endif

ifndef arch
   arch = IA32
endif

CCFLAGS = -fPIC -Wall -Wextra -D$(os) -finline-functions -O3 -fno-strict-aliasing -fvisibility=hidden

ifeq ($(arch), IA32)
   CCFLAGS += -DIA32
endif

ifeq ($(arch), POWERPC)
   CCFLAGS += -mcpu=powerpc
endif

ifeq ($(arch), SPARC)
   CCFLAGS += -DSPARC
endif

ifeq ($(arch), IA64)
   CCFLAGS += -DIA64
endif

ifeq ($(arch), AMD64)
   CCFLAGS += -DAMD64
endif

OBJS = api.o buffer.o core.o epoll.o iocp.o rdp.o recv.o send.o session.o socket.o socket_api.o test.o thread.o timer.o window.cpp
DIR = $(shell pwd)

all: librdp.so librdp.a rdp

%.o: %.cpp %.h rdp.h
	$(C++) $(CCFLAGS) $< -c

librdp.so: $(OBJS)
ifneq ($(os), OSX)
	$(C++) -shared -o $@ $^
else
	$(C++) -dynamiclib -o libudt.dylib -lstdc++ -lpthread -lm $^
endif

librdp.a: $(OBJS)
	ar -rcs $@ $^

rdp:
	cp rdp.h rdp

clean:
	rm -f *.o *.so *.dylib *.a rdp

install:
	export LD_LIBRARY_PATH=$(DIR):$$LD_LIBRARY_PATH
