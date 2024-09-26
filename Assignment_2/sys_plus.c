#include <linux/kernel.h>
#include <linux/syscalls.h>

asmlinkage long sys_plus(long x, long y)
{
        long answer = 0;
        
	answer = x + y;

        return answer;
}

SYSCALL_DEFINE2(plus, long, x, long, y)
{
        return sys_plus(x,y);
}    
