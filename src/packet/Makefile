#
# Makefile packet
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
all: clean packet.o

# If we run `make debug` instead, keep the debug symbols for gdb and define the DEBUG macro.
debug: CFLAGS += -g -DDEBUG -Wno-unused-parameter -fno-omit-frame-pointer
debug: clean packet.o

# Building packet.o
packet.o: packet.c
	@echo 'Building packet'
	@$(CC) -c -o packet.o packet.c $(CFLAGS) $(LDFLAGS)

test:
	@cd ../../tests/ && ./packetTest

.PHONY: clean

clean:
	@rm -vf *.o