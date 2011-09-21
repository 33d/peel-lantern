#include "buffer.h"
#include <unistd.h>
#include <algorithm>

//static int _skip_rows[] = { 4, 13, 22, 31 };
//static int _skip_cols[] = { 8, 17, 28 };

Buffer::Buffer(int cols, int rows,
		std::initializer_list<int> skip_cols,
		std::initializer_list<int> skip_rows) :
		skip_rows(skip_rows),
		skip_cols(skip_cols),
		rows(rows), cols(cols),
		tlc_start(1), tlc_end(13),
		out_cols(cols - skip_cols.size()),
		out_rows(rows - skip_rows.size()),
		harvesters_per_row(2),
		tlcs_per_harvester(4),
		harvester_bytes(97),
		buf((out_cols * out_rows) + (out_rows * harvesters_per_row)) {
	// Initialize the headers
	for (int i = 0; i < out_rows; i++) {
		buf[(i * 2) * harvester_bytes] = (((i / 8) * 2) << 4) | ((i % 8) << 1) | 1;
		buf[(i * 2 + 1) * harvester_bytes] = (((i / 8) * 2 + 1) << 4) | ((i % 8) << 1) | 1;
	}
}

ssize_t BufferOutput::write(int fd) {
	ssize_t count = pos - buffer.buf.size();
	ssize_t written = ::write(fd, buffer.buf.data(), count);
	pos += written;
	if (pos >= buffer.buf.size())
		pos = 0;
	return written;
}
