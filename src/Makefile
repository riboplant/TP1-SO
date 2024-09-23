CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

all: app slave view view2

father: father.c
	$(CC) $(CFLAGS) app.c -o app

son: son.c
	$(CC) $(CFLAGS) slave.c -o slave

view: view.c
	$(CC) $(CFLAGS) view.c -o view

view2: view2.c
	$(CC) $(CFLAGS) view2.c -o view2 

clean:
	rm -f app slave view view2 *.txt *.*log *.html strace_out ../strace_out
