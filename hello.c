#include <stdio.h>
int main(void)
{
	int count = 0;
	int x = 9999;
	while(x)
	{
		count++;
		x = x & (x - 1);
	}
	return count;
	printf("count=%d\n",count);
}
