#include "buffer.h"

//static int _skip_rows[] = { 4, 13, 22, 31 };
//static int _skip_cols[] = { 8, 17, 28 };

Buffer::Buffer(int cols, int rows,
		std::initializer_list<int> skip_cols,
		std::initializer_list<int> skip_rows) :
		skip_rows(skip_rows),
		skip_cols(skip_cols),
		rows(rows), cols(cols),
		// rows * 2 is because there's 2 TLCs per row
		buf((cols - skip_cols.size()) * (rows - skip_rows.size()) + (rows * 2))
{ }
