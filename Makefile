SUBDIRS = lib modules bin experiments

.PHONY: all
all: subdirs

# TACKY--there must be a better way
clean:
	make -C lib clean
	make -C modules clean
	make -C bin clean
	make -C experiments clean

.PHONY: subdirs $(SUBDIRS)
subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

modules: lib
bin: modules lib

