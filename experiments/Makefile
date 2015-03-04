CFLAGS=-Wall -pedantic

all: gen_dist_data

gen_dist_data: gen_dist_data.o
	$(CC) $(CFLAGS) -o $@ -lm $^

clean:
	rm -f gen_dist_data.o gen_dist_data

