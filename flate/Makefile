CXX = clang++
CXXFLAGS = -Wall -fPIC -O3
TARGET = libflate.so
OBJECTS = flate.o

.PHONY: clean

all: $(TARGET)

libflate.so: $(OBJECTS)
	clang++ -shared -o libflate.so flate.o
	strip $(TARGET)

%.cpp: %.o

clean:
	rm -f $(TARGET) $(OBJECTS)
