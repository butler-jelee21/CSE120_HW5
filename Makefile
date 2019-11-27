CFLAGS = -Wall
CC = gcc -g -O3
SRC=$(wildcard *.c)

matmul: $(SRC)
	$(CC) -o $@ $^ $(CFLAGS) 2>&1 | tee make.out

clean:
	@rm -f matmul make.out

submit:
	zip -r CSE120_HW5_Solution.zip Makefile matmul.c README.txt
