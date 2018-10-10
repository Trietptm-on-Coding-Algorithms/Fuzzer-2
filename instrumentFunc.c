#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <mach/mach.h>
//#include <sys/types.h>
int main (int argc,char ** argv)
{

	if (argc < 4)
	{
		puts ("Usage: instrumentFunc <file> <from> <to> [bytes inside file in hex]\n");
		return -1;
	}

	FILE * f = fopen(argv[1],"rb");
	if (f == NULL)
	{
		return -2;
		puts ("Cannot open file specified\n");
	}
	int from,to = 0;
	sscanf(argv[2],"%x",&from);
	sscanf(argv[3],"%x",&to);
	if (from >= to)
	{
		puts ("R u out of your mind ? check your range of bytes within the file... \n");
		return -3;
	}
	int fileSize = to - from;
	void * mem = mmap (NULL,fileSize,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE,fileno(f),0);
	if (mem == MAP_FAILED)
	{
		puts ("[!] Cannot allocate memory for file");
		return -4;
	}
	printf ("[-] File mapped to virtual memory : [%p]\n",mem);

	int (*pFunc)(char * str) = (int(*)(char *))(mem+from);

	sleep (1000000);

	int ret = pFunc("AAAAA");
	printf ("Returned : %d\n",ret);

	return 0;
}