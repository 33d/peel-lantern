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

template <class It> void BufferInput::addData(It start, It end) {
	for (It i = start; i < end; i++) {
		// Are we supposed to ignore this row?
		if (!buffer.skip_cols.count(in_col))
			buf.push_back(*i);
		++in_col;
		// do we have an entire half-row?
		if (buf.size() >= 48) {
			// is this a second half-row? If not, we need to load it backwards
			if (second_half_row) {
				// Are we supposed to skip this row?
				if (!buffer.skip_rows.count(in_row))
					addHalfRow(buf.begin(), buf.end());
				// Advance the current row
				++in_row;
				if (in_row > buffer.rows)
					in_row = 0;
				in_col = 0;
			} else
				// Are we supposed to skip this row?
				if (!buffer.skip_rows.count(in_row))
					addHalfRow(buf.rbegin(), buf.rend());
			buf.clear();
			second_half_row = !second_half_row;
		}
	}
}

template <class It> void addHalfRow(It start, It end) {

}

ssize_t BufferOutput::write(int fd) {
	ssize_t count = pos - buffer.buf.size();
	ssize_t written = ::write(fd, buffer.buf.data(), count);
	pos += written;
	if (pos >= buffer.buf.size())
		pos = 0;
	return written;
}
