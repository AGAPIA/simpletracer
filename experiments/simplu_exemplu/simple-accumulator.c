#include <stdio.h>
#include <string.h>

int x;
void test_simple(const unsigned char *buf) {
	int i = 1;
	if (buf[0] == 'B') {
		if (buf[1] == 'A') {
			i = 2;
		}
	}
	x = i;
}
