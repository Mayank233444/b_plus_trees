CXX = g++
CXXFLAGS = -std=c++11 -O2 -I./backend/include -pthread

TARGET = backend_server
SRC = backend/src/Server.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

test: backend/tests/BPlusTreeTests.cpp backend/src/BPlusTree.hpp
	$(CXX) $(CXXFLAGS) -I./backend/src backend/tests/BPlusTreeTests.cpp -o bplus_tree_tests
	./bplus_tree_tests

clean:
	rm -f $(TARGET) bplus_tree_tests
