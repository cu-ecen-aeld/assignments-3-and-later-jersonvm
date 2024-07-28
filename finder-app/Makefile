CC := gcc
CROSS_COMPILE ?=
TGT := writer
SRC := writer.c
OBJ := $(SRC:.c=.o)

ifdef CROSS_COMPILE
CC := $(CROSS_COMPILE)gcc
endif

$(TGT): $(OBJ)
	$(CC) -o $@ $^
	
%.o: %.c
	$(CC) -c $< -o $@
	
clean:
	rm -f $(TGT) $(OBJ)
