FLAGS=-Wno-unused-result -Wno-discarded-qualifiers
OPT_LEVEL=-Ofast

all: compile
install: compile
	cp main /usr/bin/charsh
compile:
	gcc main.c -o main $(FLAGS) $(OPT_LEVEL)
