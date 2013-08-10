# Wrappers makefile

# Path to linux include
INCLUDE_DIR=../../include/

# File list for building all wrappers
WRAPPER_SRC:= $(shell ls | grep ".c")

build_all: $(WRAPPER_SRC)
	@for w in $(WRAPPER_SRC); do gcc -I $(INCLUDE_DIR) $$w -o `echo $$w | cut -d'.' -f1`.bin ; done

buildmygetpid: test_mygetpid.c
	gcc -I $(INCLUDE_DIR) test_mygetpid.c -o test_mygetpid.bin

clean: 
	rm -rf *.bin
