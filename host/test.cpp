#include <cppunit/ui/text/TestRunner.h>
#include "buffertest.cpp"

int main(int argc, char **argv) {
	CppUnit::TextUi::TestRunner runner;
	runner.addTest(BufferTest::suite());
	runner.run();
	return 0;
}
