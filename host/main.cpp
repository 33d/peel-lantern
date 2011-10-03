#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <errno.h>
#include "buffer.h"

void check_errno(const char* function) {
	if (errno) {
		perror(function);
		exit(1);
	}
}

int main(void) {
	Buffer buf(36, 35, { 4, 13, 22, 31 }, { 8, 17, 26 },
			{ 3, 2, 1, 0, 7, 6, 5, 4 },
			{ 4, 5, 6, 7, 0, 1, 2, 3 });
	BufferInput bufIn(buf);
	BufferOutput bufOut(buf);
	uint8_t data[36 * 35];
	int nfds = std::max(STDIN_FILENO, STDOUT_FILENO) + 1;

	fd_set in, out;
	while (true) {
		FD_ZERO(&in);
		FD_ZERO(&out);
		FD_SET(STDIN_FILENO, &in);
		FD_SET(STDOUT_FILENO, &out);

		int r = select(nfds, &in, &out, NULL, NULL);
		check_errno("select");

		if (FD_ISSET(STDIN_FILENO, &in)) {
			int r = read(STDIN_FILENO, data, sizeof(data));
			check_errno("read");
			bufIn.addData(data, data + r);
		}
		if (FD_ISSET(STDOUT_FILENO, &out)) {
			int w = bufOut.write(STDOUT_FILENO);
			check_errno("write");
		}
	}

	return 0;
}
