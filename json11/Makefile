.PHONY: clean

CXX = clang++
CXXFLAGS = -fPIC -Wall -std=c++11
TARGET = libjson11.so
OBJECTS = json11.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	clang++ -shared -o $(TARGET) $(OBJECTS)

%.cpp: %.o

clean:
	rm -f $(TARGET) $(OBJECTS)
