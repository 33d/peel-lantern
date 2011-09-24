#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestSuite.h>
#include <cstdio>
#include <algorithm>
#include "buffer.h"

#include <cppunit/extensions/HelperMacros.h>

class BufferInputTest : public CppUnit::TestFixture  {
private:
	CPPUNIT_TEST_SUITE(BufferInputTest);
	CPPUNIT_TEST(testFirstRow);
	CPPUNIT_TEST(testFirstRowWithSkips);
	CPPUNIT_TEST_SUITE_END();

public:
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

	void testFirstRowWithSkips() {
		Buffer buf(36, 32, { 2, 5, 48, 49 }, {});
		CPPUNIT_ASSERT_EQUAL(4, (int) buf.skip_cols.size());
		BufferInput input(buf);

		std::vector<uint8_t> data(96);
		for (int i = 0; i < 96; ) {
			data[i++] = 0xAA;
			data[i++] = 0x66;
		}
		input.addData(data.begin(), data.end());

		int aa = BufferInput::lookup[0xaa];
		int ss = BufferInput::lookup[0x66];

		// row 1
		CPPUNIT_ASSERT_EQUAL(ss >> 4, (int) buf.buf[93]);
		// Skip the AA in row 2
		CPPUNIT_ASSERT_EQUAL((ss & 0x0F) << 4 | (ss >> 8), (int) buf.buf[92]);
		CPPUNIT_ASSERT_EQUAL(ss & 0xFF, (int) buf.buf[91]);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(BufferInputTest);
