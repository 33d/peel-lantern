OBJECTS      = buffer.o lookup.o
TEST_OBJECTS = buffertest.o test.o

CPPFLAGS     = -std=c++0x

test: $(OBJECTS) $(TEST_OBJECTS)
	$(CC) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) -lcppunit -o test \
		$(OBJECTS) $(TEST_OBJECTS)
	./test

clean:
	-rm *.o
