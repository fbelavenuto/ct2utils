# Makefile

CC = gcc
LD = gcc
CFLAGS = -g -Wall -Iinc -I../libct2/inc
LDFLAGS = 
CP = cp
RM = rm -f
MD = mkdir

ODIR = obj
SDIR = src

LIBS = ../libct2/libct2.a

SOURCES = main.c functions.c
OBJECTS = $(addprefix $(ODIR)/, $(SOURCES:.c=.o))

WCSOURCES = wav2ct2.c functions.c
WCOBJECTS = $(addprefix $(ODIR)/, $(WCSOURCES:.c=.o))

all: $(ODIR) ct2utils wav2ct2

ct2utils: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

wav2ct2: $(WCOBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(ODIR):
	$(MD) $(ODIR)

$(ODIR)/%.o : $(SDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(ODIR)/%.o : $(SDIR)/%.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

clean:
	$(RM) ct2utils wav2ct2 *.exe $(ODIR)/* core *~
