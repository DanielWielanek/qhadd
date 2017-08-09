CC    = g++
LD    = g++
#COPTS    = `root-config --cflags` -I/usr/local/root/include -g
#LDOPTS    = `root-config --libs` -g
COPTS    = -g -O  `root-config --cflags` -Wall
LDOPTS    = -g -O  `root-config --libs`  -Wall
#C++ Files
SOURCES =  qhadd.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE=qhadd
all: $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS) $(FOBJECTS)
	$(LD) -o $@ $^ $(LDOPTS)
#C++ files
.cpp.o:
	$(CC) -o $@ $^ -c $(COPTS)
clean:;         @rm -f $(OBJECTS)  $(EXECUTABLE) *.o  *.d