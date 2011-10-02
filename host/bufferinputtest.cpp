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
	CPPUNIT_TEST(testDisconnectedColumns);
	CPPUNIT_TEST(testFirstRowWithSkips);
	CPPUNIT_TEST(testInterleaving);
	CPPUNIT_TEST_SUITE_END();

public:
	void testFirstRow() {
		Buffer buf(32, 32, {}, {});
		BufferInput input(buf);
		// Add a single FF at the left
		std::vector<uint8_t> data(100);
		data[0] = 0xFF;
		data[48] = 0xFF;
		input.addData(data.begin(), data.end());

		// The top left should be 0xFF
		int val = BufferInput::lookup[0xFF];
		// the first row gets skipped
		CPPUNIT_ASSERT_EQUAL(0, (int) buf.buf[96]);
		CPPUNIT_ASSERT_EQUAL((val >> 8) & 0xFE, (int) buf.buf[95]);
		CPPUNIT_ASSERT_EQUAL(val & 0xFE, (int) buf.buf[94]);

		// now the right half-row, this is shifted in from the right
		// we start from an even row
		CPPUNIT_ASSERT_EQUAL(0, (int) buf.buf[101]);
		CPPUNIT_ASSERT_EQUAL((val >> 8) & 0xFE, (int) buf.buf[102]);
		CPPUNIT_ASSERT_EQUAL(val & 0xFE, (int) buf.buf[103]);
		CPPUNIT_ASSERT_EQUAL(0, (int) buf.buf[104]);
	}

	void testDisconnectedColumns() {
		std::vector<uint8_t> data(48);
		std::fill(data.begin(), data.end(), 0xFF);

		Buffer buf(32, 32, {}, {});
		BufferInput input(buf);
		input.addData(data.begin(), data.end());
		int val = BufferInput::lookup[0xFF];

		auto it = std::vector<uint8_t>::reverse_iterator(buf.buf.begin() + 97);
		// The first row should be empty
		CPPUNIT_ASSERT_EQUAL(0, (int) *it++);
		CPPUNIT_ASSERT_EQUAL((val >> 8) & 0xFE, (int) *it++);
		CPPUNIT_ASSERT_EQUAL(val & 0xFE, (int) *it++);
		// Go back to the start, then skip 12 entries
		it = std::vector<uint8_t>::reverse_iterator(buf.buf.begin() + 97);
		it += 18;
		CPPUNIT_ASSERT_EQUAL((val >> 4) & 0xFE, (int) *it++);
		CPPUNIT_ASSERT_EQUAL(((val & 0xF) << 4) & 0xFE, (int) *it++);
		CPPUNIT_ASSERT_EQUAL(0, (int) *it++);
		// columns 14 and 15 should be blank
		CPPUNIT_ASSERT_EQUAL(0, (int) *it++);
		CPPUNIT_ASSERT_EQUAL(0, (int) *it++);
		CPPUNIT_ASSERT_EQUAL(0, (int) *it++);
		// now we're on to the next chip
		CPPUNIT_ASSERT_EQUAL(0, (int) *it++);
		CPPUNIT_ASSERT_EQUAL((val >> 8) & 0xFE, (int) *it++);
		CPPUNIT_ASSERT_EQUAL(val & 0xFE, (int) *it++);
	}

	void testFirstRowWithSkips() {
		Buffer buf(36, 32, { 2, 15, 48, 49 }, {});
		CPPUNIT_ASSERT_EQUAL(12, (int) buf.skip_cols.size());
		//int skip_cols[] = { 6, 7, 8, 45, 46, 47, 144, 145, 146, 147, 148, 149 };
		//CPPUNIT_ASSERT(std::equal(skip_cols, skip_cols + sizeof(skip_cols), buf.skip_cols.begin()));
		BufferInput input(buf);

		std::vector<uint8_t> data(96);
		for (int i = 0; i < 96; ) {
			data[i++] = 0xAA;
			data[i++] = 0x66;
		}
		input.addData(data.begin(), data.end());

		int aa = BufferInput::lookup[0xaa];
		int ss = BufferInput::lookup[0x66];

		// 96-94 contains 00aa
		// 93-91 contains ssaa
		// 90-88 contains ssaa
		// 87-85 contains ssss because "aassaa" is skipped
		CPPUNIT_ASSERT_EQUAL((ss >> 4) & 0xFE, (int) buf.buf[87]);
		CPPUNIT_ASSERT_EQUAL((((ss & 0xF) << 4) | ((ss >> 4) & 0xF0)) & 0xFE, (int) buf.buf[86]);
		CPPUNIT_ASSERT_EQUAL(ss & 0xFE, (int) buf.buf[85]);
	}

	void testInterleaving() {
		Buffer buf(36, 32, { 2, 15, 20, 25 }, {});
		BufferInput input(buf);
		std::vector<uint8_t> data(256);
		for (int i = 0; i < data.size(); i++)
			data[i] = i;
		// Add another 6 for the skipped rows
		input.addData(data.begin(), data.end());

		// check the last pixel of the first row, which is at the start of the buffer
		auto it = buf.buf.begin() + 1 /* header */ + 3;
		int* l = BufferInput::lookup;
		CPPUNIT_ASSERT_EQUAL(0, (int) *it++);
		CPPUNIT_ASSERT_EQUAL((l[53] << 4) & 0xFE, (int) *it++);
		CPPUNIT_ASSERT_EQUAL((l[53] >> 4) & 0xFE, (int) *it++);
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(BufferInputTest);
