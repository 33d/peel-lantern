PROJECT      = peel-translate
NOTEST_OBJECTS = main.o
OBJECTS      = buffer.o lookup.o
TEST_OBJECTS = buffertest.o bufferinputtest.o test.o

CPPFLAGS     = -std=c++0x -g

.PHONY: test

all: test $(PROJECT)

$(PROJECT): $(OBJECTS) $(NOTEST_OBJECTS)
	$(CXX) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) -o $(PROJECT) $(OBJECTS) $(NOTEST_OBJECTS)

test: $(OBJECTS) $(TEST_OBJECTS)
	$(CC) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) -lcppunit -o test \
		$(OBJECTS) $(TEST_OBJECTS)
	./test

clean:
	-rm *.o
