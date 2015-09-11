CC = gcc
CFLAGS = 

TARGETS = client_tester

SOURCES = main.c

INCLUDES += -I../inc

all:$(TARGETS)

$(TARGETS): $(SOURCES)
	$(CC) $(INCLUDES) $(CFLAGS) $(LIBS) $^ -o $@

clean:
	rm -rf *.o client_tester


