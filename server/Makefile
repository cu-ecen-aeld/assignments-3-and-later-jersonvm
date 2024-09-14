CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -lpthread -lrt
INCLUDES ?=
TGT := aesdsocket
SRC := aesdsocket.c
OBJ := $(SRC:.c=.o)

all: $(TGT)

$(TGT): $(SRC)
	$(CC) $(CFLAGS) -o $(TGT) $(SRC) $(INCLUDES) $(LDFLAGS)

clean:
	rm -f $(TGT) $(OBJ)
