#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <open>

int main(int argc, char** argv)
{
	// check usage
	if (argc != 2)
	{
		fprintf(stderr, "%s <input string>\n", argv[0]);
		exit(-1);
	}

	// set the input string
	char input_string[128];
	snprintf(input_string, sizeof(input_string), "%s\n", argv[1]);

	// initialize a hash context 

	return 0;
}