#include <cppunit/TestFixture.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestSuite.h>
#include <cstdio>
#include "buffer.h"

class BufferTest : public CppUnit::TestFixture  {
private:

public:
	static CppUnit::Test *suite() {
		CppUnit::TestSuite *suite = new CppUnit::TestSuite("BufferTest");
		suite->addTest(new CppUnit::TestCaller<BufferTest>(
				"testHeadersNoSkip",
				&BufferTest::testHeadersNoSkip) );
		return suite;
	}

	void testHeadersNoSkip() {
		Buffer buf(32, 32, {}, {});
		CPPUNIT_ASSERT_EQUAL(32, buf.out_rows);
		CPPUNIT_ASSERT(buf.buf[      0] == 0x01);
		CPPUNIT_ASSERT(buf.buf[     97] == 0x11);
		CPPUNIT_ASSERT(buf.buf[ 2 * 97] == 0x03);
		CPPUNIT_ASSERT(buf.buf[ 3 * 97] == 0x13);
		CPPUNIT_ASSERT(buf.buf[14 * 97] == 0x0F);
		CPPUNIT_ASSERT(buf.buf[15 * 97] == 0x1F);
		CPPUNIT_ASSERT(buf.buf[16 * 97] == 0x21);
	}
};
