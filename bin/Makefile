MPICH2_ROOT=/opt/mpich2

CFLAGS=-I../lib -g -Wall -pedantic
LDFLAGS=-L../lib -lfg -g

MPI_CFLAGS=-I$(MPICH2_ROOT)/include
MPI_LDFLAGS=-L$(MPICH2_ROOT)/lib -lmpich -lmpl

.PHONY: all
all: mpi-test sort fg_module_index pin-array-test merge-test dsort-pass0 dsort-pass1 dsort-pass2 sort-verify param_rename_test network-copy-test network-merge-test config-test dsort-pass1-cfg dsort-pass2-cfg

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $^

mpi-test.o: mpi-test.c
	$(CC) $(CFLAGS) $(MPI_CFLAGS) -c -o $@ $^

mpi_test_objs = mpi-test.o
mpi-test: $(mpi_test_objs)
	$(CC) $(LDFLAGS) $(MPI_LDFLAGS) -o $@ $^

dsort-pass0.o: dsort-pass0.c
	$(CC) $(CFLAGS) $(MPI_CFLAGS) -c -o $@ $^

dsort_pass0_objs = dsort-pass0.o
dsort-pass0: $(dsort_pass0_objs)
	$(CC) $(LDFLAGS) $(MPI_LDFLAGS) -o $@ $^

dsort-pass1.o: dsort-pass1.c
	$(CC) $(CFLAGS) $(MPI_CFLAGS) -c -o $@ $^

dsort_pass1_objs = dsort-pass1.o
dsort-pass1: $(dsort_pass1_objs)
	$(CC) $(LDFLAGS) $(MPI_LDFLAGS) -o $@ $^

dsort-pass1-cfg.o: dsort-pass1-cfg.c
	$(CC) $(CFLAGS) $(MPI_CFLAGS) -c -o $@ $^

dsort_pass1_cfg_objs = dsort-pass1-cfg.o
dsort-pass1-cfg: $(dsort_pass1_cfg_objs)
	$(CC) $(LDFLAGS) $(MPI_LDFLAGS) -o $@ $^

dsort-pass2.o: dsort-pass2.c
	$(CC) $(CFLAGS) $(MPI_CFLAGS) -c -o $@ $^

dsort_pass2_objs = dsort-pass2.o
dsort-pass2: $(dsort_pass2_objs)
	$(CC) $(LDFLAGS) -o $@ $^

dsort-pass2-cfg.o: dsort-pass2-cfg.c
	$(CC) $(CFLAGS) -c -o $@ $^

dsort_pass2_cfg_objs = dsort-pass2-cfg.o
dsort-pass2-cfg: $(dsort_pass2_cfg_objs)
	$(CC) $(LDFLAGS) -o $@ $^

sort_objs = sort.o
sort: $(sort_objs)
	$(CC) $(LDFLAGS) -o $@ $^

param_rename_test_objs = param-rename-test.o
param_rename_test: $(param_rename_test_objs)
	$(CC) $(LDFLAGS) -o $@ $^

network_copy_test_objs = network-copy-test.o
network-copy-test: $(network_copy_test_objs)
	$(CC) $(LDFLAGS) -o $@ $^

network-merge-test.o: network-merge-test.c
	$(CC) $(CFLAGS) $(MPI_CFLAGS) -c -o $@ $^

network_merge_test_objs = network-merge-test.o
network-merge-test: $(network_merge_test_objs)
	$(CC) $(LDFLAGS) $(MPI_LDFLAGS) -o $@ $^

pin_array_test_objs = pin-array-test.o
pin-array-test: $(pin_array_test_objs)
	$(CC) $(LDFLAGS) -o $@ $^

fg_module_index_objs = fg_module_index.o
fg_module_index: $(fg_module_index_objs)
	$(CC) $(LDFLAGS) -o $@ $^

merge_test_objs = merge_test.o
merge_test: $(merge_test_objs)
	$(CC) $(LDFLAGS) -o $@ $^

sort_verify_objs = sort-verify.o
sort-verify: $(sort_verify_objs)
	$(CC) -o $@ -g $^

config_test_objs = config-test.o
config-test: $(config_test_objs)
	$(CC) $(LDFLAGS) -o $@ $^

objs = $(mpi_test_objs) $(sort_objs) $(fg_module_index_objs) \
	$(pin_array_test_objs) $(merge_test_objs) $(dsort_pass0_objs) \
	$(dsort_pass1_objs) $(dsort_pass2_objs) $(sort_verify_objs) \
	$(dsort_pass1_cfg-objs) $(dsort_pass2_cfg-objs) \
	network-copy-test.o network-merge-test.o param-rename-test.o config-test.o

.PHONY: clean
clean:
	rm -f $(objs) mpi-test sort fg_module_index pin-array-test merge-test dsort-pass0 dsort-pass1 dsort-pass2 sort-verify param_rename_test network-copy-test network-merge-test config-test dsort-pass1-cfg dsort-pass2-cfg

