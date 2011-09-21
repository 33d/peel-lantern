#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>
#include <unistd.h>
#include <set>
#include <vector>
#include <initializer_list>

class Buffer {
private:
	const std::set<int> skip_rows;
	const std::set<int> skip_cols;
	const int rows;
	const int cols;
	const int out_rows;
	const int out_cols;
	const int harvester_bytes;
	const int tlc_start; // inclusive
	const int tlc_end; // exclusive
	const int harvesters_per_row;
	const int tlcs_per_harvester;
	/** The data buffer, as sent to the slaves*/
	std::vector<uint8_t> buf;

public:
	Buffer(int cols, int rows,
			std::initializer_list<int> skip_cols,
			std::initializer_list<int> skip_rows);

friend class BufferOutput;
#if defined(CPPUNIT_ASSERT)
friend class BufferTest;
#endif
};

class BufferOutput {
private:
	const Buffer & buffer;
	int pos;
public:
	BufferOutput(const Buffer& buffer) : buffer(buffer), pos(0) {}
	/**
	 * Writes as much of the TLC buffer as possible to some file descriptor.
	 */
	ssize_t write(int fd);
};

#endif /* BUFFER_H_ */
