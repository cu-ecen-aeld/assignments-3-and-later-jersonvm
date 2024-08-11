CC := gcc
CROSS_COMPILE ?=
TGT := writer
SRC := writer.c
OBJ := $(SRC:.c=.o)
CFLAGS ?= 

ifdef CROSS_COMPILE
CC := $(CROSS_COMPILE)gcc
endif

all: $(TGT)

$(TGT): $(SRC)
	$(CC) $(CFLAGS) -o $(TGT) $(SRC)

clean:
	rm -f $(TGT) $(OBJ)
