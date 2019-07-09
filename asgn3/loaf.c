#include <stdio.h>
#include <stdlib.h>
#include <malloc_np.h>
#define PAGES (long long)(1000000) 

int main() {
	char* pile = (char*)malloc(PAGES * 4096);
	long long i, j;

	for (j = 0; j < 10; j++) {
		for (i = 0; i < (PAGES * 4096); i += 4096) {
			*(pile + i) = j;		
		}
//		printf("j %llu\n", j);
	}
	return 0;
}
