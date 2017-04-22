CC 			= gcc
CFLAGS 		= -w -std=gnu99
CSOURCES 	= lex.yy.c awk.tab.c main.c interpreter.c customstring.c customio.c
COBJECTS 	= $(CSOURCES:.c=.o) 

LEX			= flex
LEXSOURCES	= awk.lex

YACC		= bison
YACCFLAGS	= -d
YACCSOURCES	= awk.y

EXECUTABLE 	= hawk

all: main 

main: objects 
	$(CC) $(CFLAGS) $(COBJECTS) -lm -o $(EXECUTABLE) 

objects: $(SOURCES) 
	$(LEX) $(LEXSOURCES)
	$(YACC) $(YACCFLAGS) $(YACCSOURCES)
	$(CC) $(CFLAGS) -c $(CSOURCES)


clean: 
	rm -f ./*.o $(EXECUTABLE)