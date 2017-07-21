#include "stub_generator.h"
#include <stdio.h>

int main(int argc ,char ** argv)
{
	printf("argv:%s\n", argv[1]);
	stub_generator(argv[1]);

	return 0;
}