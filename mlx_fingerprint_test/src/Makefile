CLI_FILENAME = mlx_fingerprint_test
CLI_CFLAGS  = $(TARGET_CFLAGS)
CLI_LDFLAGS = $(TARGET_LDFLAGS)
CLI_LDFLAGS += -lpthread -lrt -lstdc++ -lCSAlg -lgen -luci -lzl-serial -lmlx_comm

OBJECTS = chips_fingerprint.o chips_api.o chips_sensor.o

all: $(OBJECTS) $(CLI_FILENAME)

chips_api.o: chips_api.c
	$(CC) -o $@ -c $^  $(CLI_CFLAGS) $(CLI_LDFLAGS)
	
chips_sensor.o: chips_sensor.c
	$(CC) -o $@ -c $^  $(CLI_CFLAGS) $(CLI_LDFLAGS)
	
chips_fingerprint.o: chips_fingerprint.c
	$(CC) -o $@ -c $^  $(CLI_CFLAGS) $(CLI_LDFLAGS)
	
$(CLI_FILENAME): $(OBJECTS)
	$(CC) -o $@ $^ $(CLI_CFLAGS) $(CLI_LDFLAGS)

clean:
	rm -f $(CLI_FILENAME) *.o
