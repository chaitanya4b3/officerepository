CLI_FILENAME =instancemd
CLI_CFLAGS  = $(CFLAGS)
CLI_LDFLAGS = $(LDFLAGS)

OBJECTS =instancemd.o

all: $(OBJECTS) $(CLI_FILENAME)

$(OBJECTS):%.o:%.c
	$(CC) -c $(CLI_CFLAGS) $< -o $@ 

$(CLI_FILENAME): $(OBJECTS)
	$(CC) $(CLI_LDFLAGS) -o $@ $^

clean:
	rm -f $(CLI_FILENAME) *.o
