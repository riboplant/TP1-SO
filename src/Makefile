CC = gcc
CFLAGS = -Wall -Wextra -O2

all: app slave view

father: father.c
	$(CC) $(CFLAGS) app.c -o app

son: son.c
	$(CC) $(CFLAGS) slave.c -o slave

view: view.c
	$(CC) $(CFLAGS) view.c -o view

clean:
	rm -f app slave view
