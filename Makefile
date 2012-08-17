CFLAGS += -DDEBUG
BIN = pollex

all : $(BIN)

OBJS = main.o
$(BIN) : $(OBJS)
	$(CC) $(LDFLAGS)  -o $@ $(OBJS)

clean:
	rm -f $(BIN) $(OBJS)
