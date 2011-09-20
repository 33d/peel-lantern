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
	std::vector<uint8_t> const buf;

public:
	Buffer(int cols, int rows,
			std::initializer_list<int> skip_cols,
			std::initializer_list<int> skip_rows);

friend class BufferWriter;
};

class BufferWriter {
private:
	Buffer const& buffer;
public:
	BufferWriter(Buffer& buffer) : buffer(buffer) {}
	/**
	 * Writes as much of the TLC buffer to some file descriptor as possible.
	 */
	ssize_t write(int fd) const;
};

#endif /* BUFFER_H_ */
