#
# Makefile projet1 - Reseaux Informatiques
# Octobre 2018
#
# Auteurs : Brieuc de Voghel   &   Nicolas van de Walle
# NOMA    : 59101600           &   27901600
#

CC = gcc # compilateur utilisé
CFLAGS = -g -std=c99 -Wall -W -lz -Werror -Wshadow -Wextra -lcunit # options de compilation
LDFLAGS = -lcunit # options de l'édition de liens

LIBRAIRIES = # librairies externes à utiliser (*.a) TODO ?

# liste des fichiers sources du projet
SRC_SOCKET = src/socket/create_socket.c src/socket/real_address.c src/socket/wait_for_client.c
SRC_PACKET = src/packet/packet.c
SRC = src/sender.c $(SRC_PACKET) # $(SRC_SOCKET)  src/receiver.c

OBJ = $(SRC:.c=.o) # liste des fichiers objets

EXEC = sender receiver # noms des exécutables à générer

# cible par default
all: $(EXEC)
build: $(EXEC)

cppcheck: # TODO ne fonctionne pas comme ca ... mais comment alors ?
	./cppcheck --enable=all --check-config --suppress=missingIncludeSystem $(SRC)

tests:
	@echo 'Testing (rebuilding beforehand)'
	@make clean
	@make tests/test
	./tests/test1
	@echo 'Tests complete'

tests/test: tests/*.o
	@$(CC) -o tests/test tests/*.o $(LIBRAIRIES) $(CFLAGS) $(LDFLAGS)

tests/*.o: tests/*.c
	@echo 'Building tests'
	@$(CC) -c -o tests/test.o tests/*.c $(CFLAGS) $(LDFLAGS)

$(EXEC): $(OBJ) $(LIBRAIRIES)
	@echo 'Making executable'
	@$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $(LIBRAIRIES)

# pour tout .o à construire à partir d'un .c
.o: .c
	@echo 'Building .o files'
	@$(CC) -o $@ -c $(CFLAGS)

# dépendances qui seront systématiquement reconstruites
.PHONY: build clean rebuild tests

# permet de supprimer tous les fichiers intermédiaires
clean:
	@echo 'Cleaning previously made files'
	@rm -vf $(EXEC) tests/test tests/*.o src/*.o src/packet/*.o src/socket/*.o *.o $(LIBRAIRIES)

# supprime tout et reconstruit le projet
rebuild: clean build


###########################################################################################################
# inspiration temporaire :
#
#SRC = src/final/linkedList.c
#
#SEND_SRC = src/final/read_write_loop_sender.c
#
#RECEIVE_SRC = src/final/read_write_loop_receiver.c
#
#INTERFACE = src/final/linkedList.h
#
#SEND_H = src/final/read_write_loop_sender.h
#
#RECEIVE_H = src/final/read_write_loop_receiver.h
#
#
#sender: sender.o $(SRC) $(SEND_SRC) $(INTERFACE) $(SEND_H)
#	$(CC) $(CFLAGS) sender.o $(SRC) $(SEND_SRC) -o sender -lm -lz
#
#receiver: receiver.o $(SRC) $(RECEIVE_SRC) $(INTERFACE) $(RECEIVE_H)
#	$(CC) $(CFLAGS) receiver.o $(SRC) $(RECEIVE_SRC) -o receiver -lm -lz
#
#link_sim:
#	cc -Wall -Werror -Wshadow -Wextra -O2 -std=gnu99 -c -o link_sim.o src/link_simulator/link_sim.c
#	cc -Wall -Werror -Wshadow -Wextra -O2 -std=gnu99   -c -o min_queue.o src/link_simulator/min_queue.c
#	cc   link_sim.o min_queue.o -o link_sim
#
#test: tests/tests.c $(SRC) $(INTERFACE) sender receiver link_sim
#	$(CC) -lcunit $(SRC) tests/tests.c -o test -lm $(CFLAGS)
#
#sender.o: src/final/sender.c
#	$(CC) $(CFLAGS) -c src/final/sender.c
#
#receiver.o: src/final/receiver.c
#	$(CC) $(CFLAGS) -c src/final/receiver.c
