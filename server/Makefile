CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -pthread -lrt
TGT := aesdsocket
SRC := aesdsocket.c
OBJ := $(SRC:.c=.o)

all: $(TGT)

$(TGT): $(SRC)
	$(CC) $(CFLAGS) -o $(TGT) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TGT) $(OBJ)
