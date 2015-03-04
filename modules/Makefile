MPICH2_ROOT=/opt/mpich2

CFLAGS=-I../lib -g -Wall -pedantic
LDFLAGS=

# not using pkg-config because mpich2-ch3.pc assumes C++
MPI_CFLAGS=-I$(MPICH2_ROOT)/include
MPI_LDFLAGS=-L$(MPICH2_ROOT)/lib -lmpich -lmpl -Wl,-rpath,$(MPICH2_ROOT)/lib

.PHONY: all
all: io_module.so dsort_module.so mpi_module.so

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $^

%.so: %.o
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^

mpi_module.o: mpi_module.c
	$(CC) $(CFLAGS) $(MPI_CFLAGS) -fPIC -c -o $@ $^

mpi_module.so: mpi_module.o
	$(CC) $(LDFLAGS) $(MPI_LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^

dsort_module.o: dsort_module.c
	$(CC) $(CFLAGS) $(MPI_CFLAGS) -fPIC -c -o $@ $^

dsort_module.so: dsort_module.o pq.o
	$(CC) $(LDFLAGS) $(MPI_LDFLAGS) -shared -Wl,-soname,$@ -o $@ $^

module_files = io_module.so \
	dsort_module.so \
	mpi_module.so
modules: $(module_files)

.PHONY: clean
clean:
	rm -f $(module_files) mpi_module.o pq.o dsort_module.o

