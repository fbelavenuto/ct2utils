# Makefile

CC=gcc
CPP=g++
CFLAGS=-g -Wall
LDFLAGS=

WCSOURCES=wav2ct2.cpp functions.cpp
WCOBJECTS=$(addprefix obj/, $(WCSOURCES:.cpp=.o))
WCSRCS=$(addprefix src/, $(WCSOURCES))

CWSOURCES=ct22wav.cpp functions.cpp
CWOBJECTS=$(addprefix obj/, $(CWSOURCES:.cpp=.o))
CWSRCS=$(addprefix src/, $(CWSOURCES))

BCSOURCES=bin2ct2.cpp functions.cpp
BCOBJECTS=$(addprefix obj/, $(BCSOURCES:.cpp=.o))
BCSRCS=$(addprefix src/, $(BCSOURCES))

CBSOURCES=ct22bin.cpp functions.cpp
CBOBJECTS=$(addprefix obj/, $(CBSOURCES:.cpp=.o))
CBSRCS=$(addprefix src/, $(CBSOURCES))

all: wav2ct2 ct22wav bin2ct2 ct22bin

wav2ct2: $(WCOBJECTS)
	$(CPP) $(LDFLAGS) $(WCOBJECTS) -o out/$@

ct22wav: $(CWOBJECTS)
	$(CPP) $(LDFLAGS) $(CWOBJECTS) -o out/$@

bin2ct2: $(BCOBJECTS)
	$(CPP) $(LDFLAGS) $(BCOBJECTS) -o out/$@

ct22bin: $(CBOBJECTS)
	$(CPP) $(LDFLAGS) $(CBOBJECTS) -o out/$@

obj/%.o : src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o : src/%.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

clean:
	rm -f out/* *.o obj/*.o core *~

# DO NOT DELETE
