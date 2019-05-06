IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR)
FLAGS=-g -Wall -Werror

LDIR =./lib

LIBS=-pthread

_DEPS = parser.c operations_queue.c stock_market_lib.c concurrency_layer.c
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = parser.o operations_queue.o stock_market_lib.o concurrency_layer.o
OBJ = $(patsubst %,$(LDIR)/%,$(_OBJ))


$(LDIR)/%.o: $(LDIR)/%.c $(DEPS)
	$(CC) $(FLAGS) -c -o $@ $< $(CFLAGS)

all: concurrent_market cm_1 cm_2 cm_3 cm_4

concurrent_market: concurrent_market.c $(OBJ)
	gcc $(FLAGS) -o $@ $^ $(CFLAGS) $(LIBS)

cm_1: cm_1.c $(OBJ)
	gcc $(FLAGS) -o $@ $^ $(CFLAGS) $(LIBS)

cm_2: cm_2.c $(OBJ)
	gcc $(FLAGS) -o $@ $^ $(CFLAGS) $(LIBS)

cm_3: cm_3.c $(OBJ)
	gcc $(FLAGS) -o $@ $^ $(CFLAGS) $(LIBS)

cm_4: cm_4.c $(OBJ)
	gcc $(FLAGS) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(LDIR)/*.o concurrent_market
	rm -f $(LDIR)/*.o cm_1
	rm -f $(LDIR)/*.o cm_2
	rm -f $(LDIR)/*.o cm_3
	rm -f $(LDIR)/*.o cm_4
