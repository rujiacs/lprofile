#include <stdio.h>

int func(int i) {
	int j = 10;

	j = j * (i - 1);

	return j;
}

int main()
{
	int i = 100, ret = 0;

	ret = func(i);

	fprintf(stdout, "%d\n", ret);
	
	return 0;
}
