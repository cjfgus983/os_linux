#include <linux/kernel.h>
#include <linux/syscalls.h>

asmlinkage long sys_test_reverse(char* s, long size)
{
        long start = 0;
	long end = size - 1;
	char temp;

	while(start < end)
	{
		temp = s[start];
		s[start] = s[end];
		s[end] = temp;

		start++;
		end--;
	}
        return 0;
}

SYSCALL_DEFINE2(test_reverse, char*, s, long, size) {
    return sys_test_reverse(s,size);
}       
