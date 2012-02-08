#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cstdio>
#include <iterator>
#include "buffer.h"

class BufferTest : public CppUnit::TestFixture  {
private:
	CPPUNIT_TEST_SUITE(BufferTest);
	CPPUNIT_TEST(testHeadersNoSkip);
	CPPUNIT_TEST(test_row_start);
	CPPUNIT_TEST_SUITE_END();

public:
	void testHeadersNoSkip() {
		Buffer buf(96, 32, {}, {});
		CPPUNIT_ASSERT_EQUAL(32, buf.out_rows);
		CPPUNIT_ASSERT(buf.buf[      0] == 0x01);
		CPPUNIT_ASSERT(buf.buf[     97] == 0x11);
		CPPUNIT_ASSERT(buf.buf[ 2 * 97] == 0x21);
		CPPUNIT_ASSERT(buf.buf[ 3 * 97] == 0x31);
		CPPUNIT_ASSERT(buf.buf[ 7 * 97] == 0x71);
		CPPUNIT_ASSERT(buf.buf[ 8 * 97] == 0x03);
		CPPUNIT_ASSERT(buf.buf[15 * 97] == 0x73);
	}

	void test_row_start() {
		Buffer buf(96, 32, {}, {});
		CPPUNIT_ASSERT_EQUAL((std::ptrdiff_t ) 0      , std::distance(buf.buf.begin(), buf.row_start(0, 0)));
		CPPUNIT_ASSERT_EQUAL((std::ptrdiff_t ) 97     , std::distance(buf.buf.begin(), buf.row_start(1, 0)));
		CPPUNIT_ASSERT_EQUAL((std::ptrdiff_t ) 97 *  2, std::distance(buf.buf.begin(), buf.row_start(2, 0)));
		CPPUNIT_ASSERT_EQUAL((std::ptrdiff_t ) 97 *  3, std::distance(buf.buf.begin(), buf.row_start(3, 0)));
		CPPUNIT_ASSERT_EQUAL((std::ptrdiff_t ) 97 *  7, std::distance(buf.buf.begin(), buf.row_start(7, 0)));
		CPPUNIT_ASSERT_EQUAL((std::ptrdiff_t ) 97 *  8, std::distance(buf.buf.begin(), buf.row_start(0, 1)));
		CPPUNIT_ASSERT_EQUAL((std::ptrdiff_t ) 97 *  9, std::distance(buf.buf.begin(), buf.row_start(1, 1)));
		CPPUNIT_ASSERT_EQUAL((std::ptrdiff_t ) 97 * 16, std::distance(buf.buf.begin(), buf.row_start(0, 2)));
	}
};

CPPUNIT_TEST_SUITE_REGISTRATION(BufferTest);
