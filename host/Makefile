OBJECTS      = buffer.o lookup.o
TEST_OBJECTS = test.o

CPPFLAGS     = -std=c++0x -g

.PHONY: test

all: test

test: $(OBJECTS) $(TEST_OBJECTS)
	$(CC) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) -lcppunit -o test \
		$(OBJECTS) $(TEST_OBJECTS)
	./test

clean:
	-rm *.o
