/* for complex link stuff */
#ifdef __linux__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <net/ethernet.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <linux/types.h>
#include <linux/wireless.h>
#include <netinet/if_ether.h>

/* the complex way - thanks to simple being broken on gta02 */
static int
link_ext_get(int fd, const char *name, int req, struct iwreq *rq)
{
   strncpy(rq->ifr_name, name, sizeof(rq->ifr_name));
   return (ioctl(fd, req, rq));
}

static int link_fd = -1;
static char link_ifname[1024];
static struct iw_range link_range;
static int link_has_range;

static int
link_setup(const char *name)
{
   struct iwreq rq;

   link_ifname[0] = 0;
   if (!name)
     {
	FILE *f;
	
	link_ifname[0] = 0;
	f = fopen("/proc/net/wireless", "r");
	if (f)
	  {
	     char buf[1024];
	     
	     while (fgets(buf, sizeof(buf), f))
	       {
		  if (strchr(buf, ':'))
		    {
		       char *p;
		       
		       sscanf(buf, "%s", link_ifname);
		       p = strchr(link_ifname, ':');
		       if (p) *p = 0;
		       break;
		    }
	       }
	     fclose(f);
	  }
     }
   else
     strcpy(link_ifname, name);

   if (!link_ifname[0]) return -1;
   
   link_fd = socket(AF_INET, SOCK_DGRAM, 0);
   if (link_fd < 0) return -1;
   
   link_has_range = 0;
   rq.u.data.pointer = (caddr_t)(&link_range);
   rq.u.data.length = sizeof(link_range);
   rq.u.data.flags = 1;
   
   if (link_ext_get(link_fd, link_ifname, SIOCGIWRANGE, &rq) >= 0)
     {
	link_has_range = 1;
	return 0;
     }
   close(link_fd);
   link_fd = -1;
   return -1;
}

static double
link_quality(void)
{
   struct iwreq rq;
   struct iw_statistics stats;
   
   if (link_fd < 0) return -1;
   
   rq.u.data.pointer = (caddr_t)(&stats);
   rq.u.data.length = sizeof(stats);
   rq.u.data.flags = 1;
   if (link_ext_get(link_fd, link_ifname, SIOCGIWSTATS, &rq) >= 0)
     return (double)stats.qual.qual / (double)link_range.max_qual.qual;
   close(link_fd);
   link_fd = -1;
   return -1;
}

static void
link_end(void)
{
   if (link_fd < 0) return;
   close(link_fd);
   link_fd = -1;
}

/*
static int
_wifi_signal_get(void)
{
   FILE *f;
   int strength = -1;
   char buf[4096];
   
   f = fopen("/proc/net/wireless", "r");
   if (!f) return -1;
   
   while (fgets(buf, 256, f))
     {
	int dummy;
	int count;
	int found_dev = 0;
	int wlan_status = 0;
	int wlan_link = 0;
	int wlan_level = 0;
	int wlan_noise = 0;
	char iface[64];
	
	count = sscanf(buf, "%s %i %i %i %i %i %i %i %i %i %i\n",
		       iface, &wlan_status, &wlan_link, &wlan_level, 
		       &wlan_noise, &dummy, &dummy, &dummy, &dummy, &dummy, 
		       &dummy);
	if (count != 3) continue;
	fclose(f);
	return wlan_link;
     }
   fclose(f);
   return -1;
}
*/

int
main(int argc, char **argv)
{
   int pstrength = -2;
   int strength = -1;
   int sleeptime = 8;
   const char *devname = NULL;
   
   if (argc > 1) sleeptime = atoi(argv[1]);
   if (argc > 2) devname = argv[2];
   for (;;)
     {
	pstrength = strength;
	if (link_fd < 0)
	  {
	     if (link_setup(devname) >= 0)
	       strength = link_quality() * 100.0;
	  }
	else
	  strength = link_quality() * 100.0;
	if (pstrength != strength)
	  {
	     if (strength < 0) printf("ERROR\n");
	     else printf("%i\n", strength);
	     fflush(stdout);
	  }
	sleep(sleeptime);
     }
   return 0;
}
#else
int
main(int argc, char **argv)
{
   return 0;
}
#endif
