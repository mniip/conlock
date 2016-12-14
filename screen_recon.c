#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include <sys/ptrace.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/tiocl.h>

void error(char const *str)
{
	perror(str);
	exit(-1);
}

int main(int argc, char **argv)
{
	if(-1 == setuid(0))
		error("setuid");

	if(argc < 4)
		return -1;

	unsigned long long tracee;
	if(1 > sscanf(argv[2], "%llu\n", &tracee))
		return -1;

	int fd = open(argv[1], O_RDWR);
	if(fd < 0)
		error("open");

	if(-1 == ptrace(PTRACE_ATTACH, tracee))
		error("PTRACE_ATTACH");
	if(-1 == waitpid(tracee, NULL, 0))
		error("waitpid");
	if(-1 == ptrace(PTRACE_CONT, tracee, 0, 0))
		error("PTRACE_CONT");

	long fg_console;
	char code = TIOCL_GETFGCONSOLE;
	if(-1 == (fg_console = ioctl(fd, TIOCLINUX, &code)))
		error("ioctl TIOCL_GETFGCONSOLE");
	fg_console++;

	struct vt_mode vt;
	if(-1 == ioctl(fd, VT_GETMODE, &vt))
		error("ioctl VT_GETMODE");

	long kbmode;
	if(-1 == ioctl(fd, KDGKBMODE, &kbmode))
		error("ioctl KDGKBMODE");
	if(-1 == ioctl(fd, KDSKBMODE, K_UNICODE))
		error("ioctl KDSETMODE K_UNICODE");

	long vcmode;
	if(-1 == ioctl(fd, KDGETMODE, &vcmode))
		error("ioctl KDGETMODE");
	if(-1 == ioctl(fd, KDSETMODE, KD_TEXT))
		error("ioctl KDSETMODE KD_TEXT");

	pid_t child = fork();
	if(child < 0)
		error("fork");
	if(!child)
	{
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		execvp(argv[3], argv + 3);
		error("execlp");
	}
	while(1)
	{
		int status;
		pid_t pid = wait(&status);
		if(pid == -1)
			error("wait");
		if(pid == tracee)
		{
			if(WIFEXITED(status) || WIFSIGNALED(status))
			{
				kill(child, SIGTERM);
				break;
			}
			if(WIFSTOPPED(status))
			{
				if(WSTOPSIG(status) == vt.acqsig || WSTOPSIG(status) == vt.relsig)
				{
					if(-1 == ptrace(PTRACE_CONT, tracee, 0, 0))
						error("PTRACE_CONT");
				}
				else
					if(-1 == ptrace(PTRACE_CONT, tracee, 0, WSTOPSIG(status)))
						error("PTRACE_CONT");
			}
		}
		else if(pid == child)
			break;
	}

	if(-1 == ioctl(fd, KDSKBMODE, kbmode))
		perror("ioctl KDSKBMODE restore");
	if(-1 == ioctl(fd, KDSETMODE, vcmode))
		perror("ioctl KDSETMODE restore");

	kill(tracee, vt.acqsig);
	close(fd);
	return 0;
}
