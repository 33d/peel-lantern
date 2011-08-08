#if !defined(CIRCULARBUFFER_H)
#define CIRCULARBUFFER_H

#include <stdlib.h>
#include <inttypes.h>

/**
 * A circular buffer.  It does no bounds checking.  For better performance,
 * SIZE should be a power of 2, so the compiler can perform modulo
 * instructions using a logical AND.
 */
template <class T, uint8_t SIZE>
class CircularBuffer {
public:
  CircularBuffer(): head(0), tail(0) {}
  void pushBack(T o) {
    buf[tail] = o;
    tail = (tail + 1) % SIZE;
  }
  T popFront() {
    T r = buf[head];
    head = (head + 1) % SIZE;
    return r;
  }
  uint8_t size() const {
    uint8_t tail = this->tail;
    if (tail < head)
      tail += SIZE;
    return tail - head;
  }
  T& operator[](uint8_t i) {
    return buf[(head + i) % SIZE];
  }
  uint8_t capacity() const {
	  return SIZE;
  }

private:
  T buf[SIZE];
  uint8_t head;
  uint8_t tail;
};

#endif
