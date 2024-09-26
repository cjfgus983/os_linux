#include <linux/kernel.h>
#include <linux/syscalls.h>

asmlinkage long sys_test_minus(long x, long y, long* z)
{
	long ans = x - y;
	*z = ans;
	return 0;
}

SYSCALL_DEFINE3(test_minus, long, x, long, y, long*, z) {
    return sys_test_minus(x,y,z);
}

