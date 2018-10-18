#
# Makefile projet1 - Reseaux Informatiques
# Octobre 2018
#
# Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
# NOMA    : 59101600           &   27901600
#
# Contenu inspire de l'exercice preparatoire au projet : https://inginious.info.ucl.ac.be/course/LINGI1341/envoyer-et-recevoir-des-donnees
#

CC = gcc # compilateur utilisé
CFLAGS += -g -W -lz -lcunit # options de compilation
CFLAGS += -std=c99 # Define which version of the C standard to use
CFLAGS += -Wall # Enable the 'all' set of warnings
CFLAGS += -Werror # Treat all warnings as error
CFLAGS += -Wshadow # Warn when shadowing variables
CFLAGS += -Wextra # Enable additional warnings
CFLAGS += -O2 -D_FORTIFY_SOURCE=2 # Add canary code, i.e. detect buffer overflows
CFLAGS += -fstack-protector-all # Add canary code to detect stack smashing
CFLAGS += -D_POSIX_C_SOURCE=201112L -D_XOPEN_SOURCE # feature_test_macros for getpot and getaddrinfo

LDFLAGS = -rdynamic

# fichiers sources du projet
SRC = src/packet/packet.o src/stack/stack.o src/socket/create_socket.o src/socket/read_write_loop.o src/socket/read_write_loop_receiver.o src/socket/real_address.o src/socket/wait_for_client.o

# noms des exécutables à générer
EXEC = sender_receiver # create_tests


all: clean $(EXEC)
build: $(EXEC)

cppcheck:
	cppcheck --enable=all --check-config --suppress=missingIncludeSystem src/sender.c src/receiver.c src/packet/packet.c rc/stack/stack.c src/socket/create_socket.c src/socket/read_write_loop.c src/socket/real_address.c src/socket/wait_for_client.c

sender_receiver: create_socket create_packet create_stack
	@echo 'Making executable'
	@$(CC) -c -o src/sender.o src/sender.c $(CFLAGS) $(LDFLAGS)
	@$(CC) src/sender.o $(SRC) -o sender $(CFLAGS) $(LDFLAGS)
	@$(CC) -c -o src/receiver.o src/receiver.c $(CFLAGS) $(LDFLAGS)
	@$(CC) src/receiver.o $(SRC) -o receiver $(CFLAGS) $(LDFLAGS)

create_socket:
	@cd src/socket && $(MAKE) -s

create_packet:
	@cd src/packet && $(MAKE) -s

create_stack:
	@cd src/stack && $(MAKE) -s

create_tests:
	@cd tests && $(MAKE) -s

test: clean build create_tests
	@cd src/stack && $(MAKE) -s test

# dépendances qui seront systématiquement reconstruites
.PHONY: build clean rebuild

# permet de supprimer tous les fichiers intermédiaires
clean:
	@echo 'Cleaning previously made files'
	@rm -vf sender receiver tests/test tests/*.o src/*.o src/packet/*.o src/stack/*.o *.o
	@cd src/socket && $(MAKE) -s clean
	@cd src/packet && $(MAKE) -s clean
	@cd src/stack && $(MAKE) -s clean
	@cd tests && $(MAKE) -s clean


#// TODO test EXTENSIVELY with : valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./main_to_execute