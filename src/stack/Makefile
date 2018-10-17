#
# Makefile stack
# Octobre 2018
#
# Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
# NOMA    : 59101600           &   27901600
#
# Contenu repris et modifie de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/envoyer-et-recevoir-des-donnees
#

CC = gcc -g -lz

# See gcc/clang manual to understand all flags
CFLAGS += -g -W -lz -lcunit # options de compilation
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

# We have no libraries to link against except libc, but we want to keep the symbols for debugging
LDFLAGS= -rdynamic

# Default target
all: clean stack.o

# If we run `make debug` instead, keep the debug symbols for gdb and define the DEBUG macro.
debug: CFLAGS += -g -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer
debug: clean stack.o

# Building stack.o
stack.o: stack.c
	@echo 'Building stack'
	@$(CC) -c -o stack.o stack.c $(CFLAGS) $(LDFLAGS)

test: clean stack.o
	@$(CC) -c -o ../../tests/stackTest.o ../../tests/stackTest.c $(CFLAGS) $(LDFLAGS)
	@$(CC) ../../tests/stackTest.o -o ../../tests/stackTest $(CFLAGS) $(LDFLAGS)
	@cd ../../tests/ && ./stackTest


.PHONY: clean

clean:
	@rm -vf *.o