#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>
#include <unistd.h>
#include <set>
#include <vector>
#include <initializer_list>
#include <array>
#include <cstdio>

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
	const std::vector<int> row_mapping_even;
	const std::vector<int> row_mapping_odd;

public:
	Buffer(int cols, int rows,
			std::initializer_list<int> skip_cols,
			std::initializer_list<int> skip_rows,
			std::initializer_list<int> = std::initializer_list<int>({ 0, 1, 2, 3, 4, 5, 6, 7 }),
			std::initializer_list<int> = std::initializer_list<int>({ 0, 1, 2, 3, 4, 5, 6, 7 }));
	std::vector<uint8_t>::iterator row_start(int tlc, int row);

friend class BufferOutput;
friend class BufferInput;

#if defined(CPPUNIT_ASSERT)
friend class BufferTest;
friend class BufferInputTest;
#endif
};

class BufferInput {
private:
	int in_row;
	int in_col;
	int out_row;
	Buffer& buffer;
	/** How many 8-bit input values make a half row */
	const int input_half_row;
	std::vector<uint8_t> buf;
	template <class It> void addHalfRow(const It& start);
	template <class InIt, class OutIt>
		void loadHalfRow(const InIt& start, const OutIt& pos, int startCol);
	bool second_half_row;
	static int lookup[];
public:
	BufferInput(Buffer& buf) : in_row(0), in_col(0), buffer(buf),
		second_half_row(false), out_row(0),
		input_half_row((buffer.cols - buffer.skip_cols.size()) / 2) {}
	template <class It> void addData(const It start, const It end);

#if defined(CPPUNIT_ASSERT)
friend class BufferInputTest;
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

template <class It> void BufferInput::addData(const It start, const It end) {
	for (It i = start; i < end; i++) {
		// Are we supposed to ignore this row?
		if (!buffer.skip_cols.count(in_col))
			buf.push_back(*i);
		++in_col;
		// do we have an entire half-row?
		if (buf.size() >= input_half_row) {
			// Are we supposed to skip this row?
			if (!buffer.skip_rows.count(in_row))
				addHalfRow(buf.begin());
			// is this a second half-row? Advance the row if so
			if (second_half_row) {
				++in_row;
				if (in_row > buffer.rows)
					in_row = 0;
				in_col = 0;
			}
			buf.clear();
			second_half_row = !second_half_row;
		}
	}
}

template <class It> void BufferInput::addHalfRow(const It& start) {
	int tlc = out_row / 2 * 8 + (second_half_row ? 1 : 0);
	int row = out_row % 8;
	// convert this to the mapped row
	row = second_half_row ? buffer.row_mapping_odd[row] : buffer.row_mapping_even[row];
	std::vector<uint8_t>::iterator pos = buffer.row_start(tlc, row);
	if (second_half_row) {
		// A right half-row.  The data in memory is in the same order as it
		// appears on the lantern.
		int col = (16 - buffer.tlc_end) * 3 / 2;
		pos += col;
		// skip the header byte
		++pos;
		loadHalfRow(start, pos, buffer.tlc_start);

		// go to the next row
		++out_row;
		if (out_row >= buffer.out_rows)
			out_row = 0;
	} else {
		// A left half-row.  The data in memory is the reverse of how it
		// appears on the lantern.
		// Start at the beginning of the next row (reverse_iterator points to the
		// element BEFORE the one you give it)
		pos += 97;
		// skip the start of the row
		pos -= buffer.tlc_start * 3 / 2;
		std::printf("%02d %02d %03d\n", tlc, row,
				pos - buffer.buf.begin());
		loadHalfRow(start, std::vector<uint8_t>::reverse_iterator(pos), buffer.tlc_start);
	}
}

template <class InIt, class OutIt>
	void BufferInput::loadHalfRow(const InIt& start, const OutIt& it, int startCol) {
	OutIt pos(it);
	int col = startCol;
	for (InIt i = start; i < start + input_half_row; i++) {
		uint16_t val = lookup[*i];
		if (col & 1) { // an odd column
			// remember to clear the low bit, otherwise the slave thinks this
			// is an address
			*pos |= (val >> 8) & 0xFE;
			++pos;
			*pos = (uint8_t) val & 0xFE;
			++pos;
		} else { // an even column
			*pos = (val >> 4) & 0xFE;
			++pos;
			*pos = (val & 0xF) << 4;
		}

		++col;
		// have we reached the end of this TLC?
		if ((col & 0x0F) >= buffer.tlc_end) {
			// advance to the next TLC
			col = (col & ~0xF) + 16 + buffer.tlc_start;
			pos += (16 - buffer.tlc_end + buffer.tlc_start) * 3 / 2;
		}
	}
}

#endif /* BUFFER_H_ */
