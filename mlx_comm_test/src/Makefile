CLI_FILENAME = mlx_comm_test
CLI_CFLAGS  = $(CFLAGS)
CLI_LDFLAGS = $(LDFLAGS)

OBJECTS = mlx_comm_test.o

all: $(OBJECTS) $(CLI_FILENAME)

$(OBJECTS):%.o:%.c
	$(CC) -c $(CLI_CFLAGS) $< -o $@ 

$(CLI_FILENAME): $(OBJECTS)
	$(CC) $(CLI_LDFLAGS) -o $@ $^

clean:
	rm -f $(CLI_FILENAME) *.o
