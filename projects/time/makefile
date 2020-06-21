CC=gcc
CFLAGS= `sdl2-config --libs --cflags` -I. -g3 
DEPS =
NAME = time
OBJ = timelft.o 

.PHONY = $(NAME)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
