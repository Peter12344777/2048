.PHONY: all
.PHONY: clean

all: hello2048.exe

clean: hello2048.exe
	rm $^

hello2048.exe: hello2048.c
	gcc -g $^ -o $@

