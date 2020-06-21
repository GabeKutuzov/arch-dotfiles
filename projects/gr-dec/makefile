CC=gcc
CFLAGS=-I. -g3
DEPS =
NAME = TCP-GRDecode
OBJ = main.o 

.PHONY = $(NAME)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(NAME): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

