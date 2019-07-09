all: argshell.c 
	 flex shell.l
	 cc -g -o argshell argshell.c lex.yy.c -lfl
clean: 
	rm lex.yy.c 
spotless: 
	rm lex.yy.c argshell 
