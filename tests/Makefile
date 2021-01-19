# Files
BIN_FILE = merge_sort
OBJ_FILES = merge_sort.o
# Compiler
CC        = cc
WARNINGS  = -Wall -Wextra
ERRORS    = -Werror=return-type
CFLAGS    = -march=native -O2 -std=c99 -pedantic $(WARNINGS) $(ERRORS)
LDFLAGS   = -L../ -lallocator -Wl,-rpath=../

all: $(BIN_FILE)
	@echo -e "\n* Tests built successfully *\n"

$(BIN_FILE): $(OBJ_FILES)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(OBJ_FILES) $(BIN_FILE)

.PHONY: all clean