CC      = gcc
CFLAGS  = -Wall -Wextra -g -I./include
LDFLAGS = -lpthread

SRCS = src/main.c       \
       src/tablero.c    \
       src/jugador.c    \
       src/visualizacion.c

OBJS = $(SRCS:src/%.c=obj/%.o)
TARGET = parchis

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo ">>> Compilación exitosa: ./$(TARGET)"

obj/%.o: src/%.c | obj
	$(CC) $(CFLAGS) -c $< -o $@

obj:
	mkdir -p obj

clean:
	rm -f obj/*.o $(TARGET)
	@echo ">>> Limpieza completa"
