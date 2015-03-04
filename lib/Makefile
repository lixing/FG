# not using pkg-config because mpich2-ch3.pc assumes C++
CFLAGS=-Wall -g -pedantic -pthread
LDFLAGS=-pthread

.PHONY: all
all: libfg.so

%.o: %.c FG.h fg_internal.h
	$(CC) -fPIC $(CFLAGS) -c -o $@ $<

libfg_objs = fg.o \
			 fg_network.o \
			 fg_network_config.o \
			 fg_stage.o \
			 fg_pin.o \
			 fg_queue.o \
			 fg_buffer.o
libfg.so: $(libfg_objs)
	$(CC) $(LDFLAGS) -ldl -shared -Wl,-soname,$@ -o $@ $^

libfg.a: $(libfg_objs)
	$(AR) rcs $@ $^

objs = $(libfg_objs)

.PHONY: clean
clean:
	rm -f $(objs) libfg.so

