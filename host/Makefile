PROJECT      = peel-translate
OBJECTS      = buffer.o lookup.o
PROJECT_OBJECTS = main.o $(OBJECTS)
TEST_OBJECTS = buffertest.o bufferinputtest.o test.o $(OBJECTS)

CPPFLAGS    += -std=c++0x -g

.PHONY: test

all: test $(PROJECT)

$(PROJECT): $(PROJECT_OBJECTS)
	$(CXX) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) -o $(PROJECT) $(PROJECT_OBJECTS)

test: $(OBJECTS) $(TEST_OBJECTS)
	$(CXX) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) $(TEST_OBJECTS) -lcppunit -o test
	./test

clean:
	-rm *.o
