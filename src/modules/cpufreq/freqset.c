#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

int
main(int argc, char *argv[])
{
   int new_frequency = 0;
   FILE *f;
   
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
        new_frequency = atoi(argv[2]);
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
        new_frequency = atoi(argv[2]);
	
        f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", "w");
        if (!f)
          {
             fprintf(stderr, "Unable to open frequency interface for writing.\n");
             return 1;
          }
        fprintf(f, "%d\n", new_frequency);
        fclose(f);
	
        return 0;
     }
   else if (!strcmp(argv[1], "governor"))
     {
        f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "w");
        if (!f)
          {
             fprintf(stderr, "Unable to open governor interface for writing.\n");
             return 1;
          }
        fprintf(f, "%s\n", argv[2]);
        fclose(f);
	
        return 0;
     }
   else
     {
        fprintf(stderr, "Unknown command.\n");
        return 1;
     }
#endif

   seteuid(-1);
}
