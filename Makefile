# Makefile

CC = gcc
CPP = g++
CFLAGS = -g -Wall
LDFLAGS = 
CP = cp
RM = rm -f
MD = mkdir

ODIR = obj
SDIR = src

WCSOURCES=wav2ct2.cpp functions.cpp
WCOBJECTS=$(addprefix $(ODIR)/, $(WCSOURCES:.cpp=.o))
WCSRCS=$(addprefix $(SDIR)/, $(WCSOURCES))

CWSOURCES=ct22wav.cpp functions.cpp
CWOBJECTS=$(addprefix $(ODIR)/, $(CWSOURCES:.cpp=.o))
CWSRCS=$(addprefix $(SDIR)/, $(CWSOURCES))

BCSOURCES=bin2ct2.cpp functions.cpp
BCOBJECTS=$(addprefix $(ODIR)/, $(BCSOURCES:.cpp=.o))
BCSRCS=$(addprefix $(SDIR)/, $(BCSOURCES))

CBSOURCES=ct22bin.cpp functions.cpp
CBOBJECTS=$(addprefix $(ODIR)/, $(CBSOURCES:.cpp=.o))
CBSRCS=$(addprefix $(SDIR)/, $(CBSOURCES))

all: $(ODIR) wav2ct2 ct22wav bin2ct2 ct22bin

wav2ct2: $(WCOBJECTS)
	$(CPP) $(LDFLAGS) $(WCOBJECTS) -o $@

ct22wav: $(CWOBJECTS)
	$(CPP) $(LDFLAGS) $(CWOBJECTS) -o $@

bin2ct2: $(BCOBJECTS)
	$(CPP) $(LDFLAGS) $(BCOBJECTS) -o $@

ct22bin: $(CBOBJECTS)
	$(CPP) $(LDFLAGS) $(CBOBJECTS) -o $@

$(ODIR):
	$(MD) $(ODIR)

$(ODIR)/%.o : $(SDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/%.o : $(SDIR)/%.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

clean:
	$(RM) wav2ct2 ct22wav bin2ct2 ct22bin *.exe $(ODIR)/* core *~
