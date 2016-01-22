CXX = clang++
CXXFLAGS = -Wall -fPIC -std=c++11 -I../flate -I../json11 -O3
TARGET = liblali.so
OBJECTS = web.o config.o http_request.o

.PHONY: clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -shared -o $(TARGET) -L../flate -lflate -L../json11 -ljson11 $(OBJECTS)
	strip $(TARGET)

%.cpp: %.o

clean:
	rm -f *.o $(TARGET)
