#Copyright© 2023 Nastase Cristian-Gabriel 315CAa

CC=gcc
CFLAGS=-I -Wall -Wextra -std=c99

build:
	gcc vma.c -c
	gcc vma.o main.c -o vma

run_vma:
	./vma -Wall -Wextra -std=c99

clean:
	rm -f vma vma.o