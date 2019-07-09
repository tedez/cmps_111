Files in Directory: 
	Makefile
	argshell.c
	shell.l
	DESIGN_DOC.txt
	README.txt

Purpose of each file:
	Makefile:
		Compiles necessary files into argshell exectuable.
	argshell.c:
		The shell I wrote for the assignment. Handles the commands and return values of FreeBSD OS.
	shell.l:
		Lex parser, code was provided and not modified. 
	DESIGN_DOC.txt:
		Description of the functions in 'argshell.c' and why I chose my implementation.
	README.txt:
		This file, used to describe files in the /tdersch/asgn1 directory.

RUNNING argshell.c:
	make          - will build argshell
	./argshell    - will run argshell
	make clean    - will rm lex.yy.c
	make spotless - will rm both 'lex.yy.c' and the executable 'argshell'
