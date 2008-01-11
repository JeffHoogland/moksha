/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

static int sys_cpu_setall(const char *control, const char *value);

int
main(int argc, char *argv[])
{
   if (argc != 3)
     {
        fprintf(stderr, "Invalid command. Syntax:\n");
        fprintf(stderr, "\tfreqset <frequency|governor> <freq-level|governor-name>\n");
        return 1;
     }
   
   if (seteuid(0))
     {
        fprintf(stderr, "Unable to assume root privileges\n");
     }

#ifdef __FreeBSD__
   if (!strcmp(argv[1], "frequency"))
     {
        int new_frequency = atoi(argv[2]);
	int len = 4;	
        if (sysctlbyname("dev.cpu.0.freq", NULL, 0, &new_frequency, &len) == -1)
          {
             fprintf(stderr, "Unable to open frequency interface for writing.\n");
             return 1;
          }
	
        return 0;
     }
   else if (!strcmp(argv[1], "governor"))
     {
        fprintf(stderr, "Governors not (yet) implemented on FreeBSD.\n");
        return 0;
     }
   else
     {
        fprintf(stderr, "Unknown command.\n");
        return 1;
     }
#else   
   if (!strcmp(argv[1], "frequency"))
     {
	if (sys_cpu_setall("scaling_setspeed", argv[2]) == 0)
          {
             fprintf(stderr, "Unable to open frequency interface for writing.\n");
             return 1;
          }
	
        return 0;
     }
   else if (!strcmp(argv[1], "governor"))
     {
	if (sys_cpu_setall("scaling_governor", argv[2]) == 0)
          {
             fprintf(stderr, "Unable to open governor interface for writing.\n");
             return 1;
          }
	
        return 0;
     }
   else
     {
        fprintf(stderr, "Unknown command.\n");
        return 1;
     }
#endif

   seteuid(-1);
   return -1;
}

static int
sys_cpu_setall(const char *control, const char *value)
{
   int	num = 0;
   char	filename[4096];
   FILE	*f;

   while (1)
     {
	snprintf(filename, sizeof(filename), "/sys/devices/system/cpu/cpu%i/cpufreq/%s", num, control);
	f = fopen(filename, "w");

	if (!f)
	  {
	     return(num);
	  }
	fprintf(f, "%s\n", value);
	fclose(f);
	num++;
     }
   return -1;
}
