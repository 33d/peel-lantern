#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdint.h>
#include <unistd.h>
#include <set>
#include <vector>
#include <initializer_list>
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

public:
	Buffer(int cols, int rows,
			std::initializer_list<int> skip_cols,
			std::initializer_list<int> skip_rows);

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
	std::vector<uint8_t> buf;
	template <class It> void addHalfRow(const It start);
	template <class InIt, class OutIt>
		void loadHalfRow(const InIt& start, const OutIt& pos, int startCol);
	bool second_half_row;
	static int lookup[];
public:
	BufferInput(Buffer& buf) : in_row(0), in_col(0), buffer(buf),
		second_half_row(false), out_row(0) {}
	template <class It> void addData(It start, It end);

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

template <class It> void BufferInput::addData(It start, It end) {
	for (It i = start; i < end; i++) {
		// Are we supposed to ignore this row?
		if (!buffer.skip_cols.count(in_col))
			buf.push_back(*i);
		++in_col;
		// do we have an entire half-row?
		if (buf.size() >= 48) {
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

template <class It> void BufferInput::addHalfRow(const It start) {
	// 97 bytes in the output buffer for a half-row
	int pos = out_row * 97 * 2;
	if (second_half_row) {
		// A right half-row.  The data in memory is in the same order as it
		// appears on the lantern.
		pos += 97;
		int col = (16 - buffer.tlc_end) * 3 / 2;
		pos += col;
		// skip the header byte
		++pos;
		loadHalfRow(start, buffer.buf.begin() + pos, buffer.tlc_start);

		// go to the next row
		++out_row;
		if (out_row > buffer.out_rows)
			out_row = 0;
	} else {
		// A left half-row.  The data in memory is the reverse of how it
		// appears on the lantern.
		int col = buffer.tlc_start * 3 / 2;
		// start at the end of the row
		pos += 96;
		pos -= col;
		loadHalfRow(start, buffer.buf.rend() - 1 - pos, buffer.tlc_start);
	}
}

template <class InIt, class OutIt>
	void BufferInput::loadHalfRow(const InIt& start, const OutIt& it, int startCol) {
	OutIt pos(it);
	int col = startCol;
	for (InIt i = start; i < start + 48; i++) {
		uint16_t val = lookup[*i];
		if (col & 1) { // an odd column
			*pos |= val >> 8;
			++pos;
			*pos = (uint8_t) val;
			++pos;
		} else { // an even column
			*pos = val >> 4;
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
