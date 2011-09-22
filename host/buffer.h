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
friend class BufferInput;

#if defined(CPPUNIT_ASSERT)
friend class BufferTest;
#endif
};

class BufferInput {
private:
	int in_row;
	int in_col;
	int out_row;
	Buffer& buffer;
	std::vector<uint8_t> buf;
	template <class It> void addHalfRow(It start, It end);
	bool second_half_row;
public:
	BufferInput(Buffer& buf) : in_row(0), in_col(0), buffer(buf),
		second_half_row(false), out_row(0) {}
	template <class It> void addData(It start, It end);
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
