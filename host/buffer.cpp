#include "buffer.h"
#include <unistd.h>
#include <algorithm>

//static int _skip_rows[] = { 4, 13, 22, 31 };
//static int _skip_cols[] = { 8, 17, 28 };

static std::set<int> create_skips(std::initializer_list<int> list) {
	std::set<int> r;
	for (auto i = list.begin(); i < list.end(); i++) {
		int base = (*i) * 3;
		r.insert(base++);
		r.insert(base++);
		r.insert(base++);
	}
	return r;
}

Buffer::Buffer(int cols, int rows,
		std::initializer_list<int> const& skip_cols,
		std::initializer_list<int> const& skip_rows,
		std::initializer_list<int> const& row_mapping_even,
		std::initializer_list<int> const& row_mapping_odd) :
		skip_rows(create_skips(skip_rows)),
		skip_cols(create_skips(skip_cols)),
		rows(rows), cols(cols * 3),
		tlc_start(1), tlc_end(13),
		out_cols(cols - skip_cols.size()),
		out_rows(rows - skip_rows.size()),
		harvesters_per_row(2),
		tlcs_per_harvester(4),
		harvester_bytes(97),
		buf(out_rows * harvesters_per_row * harvester_bytes),
		row_mapping_odd(row_mapping_odd),
		row_mapping_even(row_mapping_even) {
	// Initialize the headers
	for (int i = 0; i < out_rows; i++) {
		buf[(i * 2) * harvester_bytes] = (((i*2) % 8) << 4) | ((i / 4) << 1) | 1;
		buf[(i * 2 + 1) * harvester_bytes] = ((((i*2) % 8) + 1) << 4) | ((i / 4) << 1) | 1;
	}
}

ssize_t BufferOutput::write(int fd) {
	ssize_t count = buffer.buf.size() - pos;
	ssize_t written = ::write(fd, buffer.buf.data() + pos, count);
	pos += written;
	if (pos >= buffer.buf.size())
		pos = 0;
	return written;
}

std::vector<uint8_t>::iterator Buffer::row_start(int tlc, int row) {
	return buf.begin() + (row * 8 + tlc) * 97;
}
