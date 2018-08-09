SOURCE=main.cc txtparser.cc txtparser.h  

MYPROGRAM=a.out

CC=g++

LIB_SUFFIX=/usr/local/lib


all: $(MYPROGRAM)
$(MYPROGRAM): $(SOURCE)
	$(CC) $(SOURCE) $(LIB_SUFFIX)/libhiredis.a -o $(MYPROGRAM)


clean:
	rm -f $(MYPROGRAM)