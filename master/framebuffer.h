#if !defined(__FRAMEBUFFER_H)
#define __FRAMEBUFFER_H

class Framebuffer {
private:
	uint8_t buf[32 * 32 * 3];
	uint8_t* next;
	void init();

public:
	Framebuffer(): next(buf) {
		init();
	}

	void handle(uint8_t data);
};

extern Framebuffer fb;

#endif
