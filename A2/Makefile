# Nome do executável de teste
EXEC = vina

# Arquivos-fonte
SRCS = main.c vina.c lz.c

# Arquivos-objeto (substitui .c por .o)
OBJS = $(SRCS:.c=.o)

# Compilador e flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Regra padrão
all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

# Regra para remover arquivos gerados
clean:
	rm -f $(EXEC) *.o *.vc

# Compilação individual dos objetos
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
