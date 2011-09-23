#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestSuite.h>
#include <cstdio>
#include "buffer.h"

class BufferInputTest : public CppUnit::TestFixture  {
private:

public:
	static CppUnit::Test *suite() {
		CppUnit::TestSuite *suite = new CppUnit::TestSuite("BufferTest");
		suite->addTest(new CppUnit::TestCaller<BufferInputTest>(
				"testFirstRow",
				&BufferInputTest::testFirstRow) );
		return suite;
	}

	void testFirstRow() {
		Buffer buf(32, 32, {}, {});
		BufferInput input(buf);
		// Add a single FF at the left
		std::vector<uint8_t> data(96);
		data[0] = 0xFF;
		data[48] = 0xFF;
		input.addData(data.begin(), data.end());

		// The top left should be 0xFF
		int val = BufferInput::lookup[0xFF];
		// the first row gets skipped
		CPPUNIT_ASSERT_EQUAL(0, (int) buf.buf[96]);
		CPPUNIT_ASSERT_EQUAL(val >> 8, (int) buf.buf[95]);
		CPPUNIT_ASSERT_EQUAL(val & 0xFF, (int) buf.buf[94]);

		// now the right half-row, this is shifted in from the right
		// we start from an even row
		CPPUNIT_ASSERT_EQUAL(0, (int) buf.buf[101]);
		CPPUNIT_ASSERT_EQUAL(val >> 8, (int) buf.buf[102]);
		CPPUNIT_ASSERT_EQUAL(val & 0xFF, (int) buf.buf[103]);
		CPPUNIT_ASSERT_EQUAL(0, (int) buf.buf[104]);
	}
};
