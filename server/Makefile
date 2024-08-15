CC := gcc
CROSS_COMPILE ?=
TGT := aesdsocket
SRC := aesdsocket.c
OBJ := $(SRC:.c=.o)
CFLAGS += -g -Wall -Werror

ifdef CROSS_COMPILE
CC := $(CROSS_COMPILE)gcc
endif

all: $(TGT)

$(TGT): $(SRC)
	$(CC) $(CFLAGS) -o $(TGT) $(SRC)

clean:
	rm -f $(TGT) $(OBJ)
