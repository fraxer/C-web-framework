	#include <stdio.h>

// void* view(request_t* request);

void* view(void* request) {
	printf("run view handler, %s\n", (char*)request);
	return 0;
}