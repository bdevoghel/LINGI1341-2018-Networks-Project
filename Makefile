# Makefile

CC = gcc # compilateur utilisé
CFLAGS = -g -Wall -W # options de compilation
LDFLAGS = -lcunit # options de l'édition de liens
LIBRAIRIES = # librairies externes à utiliser (*.a) TODO ?
SRC = main.c # liste des fichiers sources du projet
OBJ = $(SRC:.c=.o) # liste des fichiers objets
EXEC = sender reciever # noms des exécutables à générer

build: $(EXEC) # cible par default

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
	@$(CC) -c -o tests/test.o tests/*.c $(CFLAGS)

$(EXEC): $(OBJ) $(LIBRAIRIES)
	@echo 'Making executable'
	@$(CC) -o $@ $^ $(LDFLAGS) $(CFLAGS) $(LIBRAIRIES)

# pour main (et tout autre .c à construire à partir d'un .o)
%.o: %.c
	@echo 'Building main'
	@$(CC) -o $@ -c $< $(CFLAGS)


# dépendances qui seront systématiquement reconstruites
.PHONY: build clean rebuild tests

# permet de supprimer tous les fichiers intermédiaires
clean:
	@echo 'Cleaning previously made files'
	@rm -vf $(EXEC) tests/test tests/*.o *.o $(LIBRAIRIES)

# supprime tout et reconstruit le projet
rebuild: clean build
