# Makefile - Gerenciador de Processos
#
# Alvos principais:
#   make          → compila o projeto em bin/app
#   make clean    → remove objetos e binário
#   make run      → compila e executa

CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 -D_DEFAULT_SOURCE -Iinc

SRCDIR  := src
OBJDIR  := obj
BINDIR  := bin
TARGET  := $(BINDIR)/app

SRCS    := $(wildcard $(SRCDIR)/*.c)
OBJS    := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR) $(OBJDIR):
	mkdir -p $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)

run: all
	./$(TARGET)