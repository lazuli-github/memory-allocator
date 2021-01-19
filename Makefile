# Files
LIB_FILE        = liballocator.a
SHARED_LIB_FILE = liballocator.so
OBJ_FILES       = allocator.o
# Compiler
CC        = cc
WARNINGS  = -Wall -Wextra
ERRORS    = -Werror=return-type
CFLAGS    = -march=native -O2 -std=gnu99 -pedantic -fPIC $(WARNINGS) $(ERRORS)

all: $(LIB_FILE) $(SHARED_LIB_FILE)
	@echo -e "\n*** Project built successfully ***\n"

$(LIB_FILE): $(OBJ_FILES)
	$(AR) rcs $@ $^

$(SHARED_LIB_FILE): $(OBJ_FILES)
	$(CC) -shared $^ -o $@

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

tests:
	$(MAKE) -C tests/

clean:
	rm -f $(OBJ_FILES) $(LIB_FILE) $(SHARED_LIB_FILE)
	$(MAKE) -C tests/ clean

.PHONY: all clean tests