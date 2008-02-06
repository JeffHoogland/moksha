/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

static int sensor_type = SENSOR_TYPE_NONE;
static char *sensor_name = NULL;
static int poll_interval = 32;
static int cur_poll_interval = 32;

static char *sensor_path = NULL;
#ifdef __FreeBSD__
static int mib[5];
#endif
static Ecore_Poller *poller = NULL;
static int ptemp = 0;

static void init(void);
static int check(void);
static int poll_cb(void *data);

Ecore_List *
temperature_get_bus_files(const char* bus)
{
   Ecore_List *result;
   Ecore_List *therms;
   char        path[PATH_MAX];
   char		busdir[PATH_MAX];
   
   result = ecore_list_new();
   if (result)
     {
	ecore_list_free_cb_set(result, free);
	snprintf(busdir, sizeof(busdir), "/sys/bus/%s/devices", bus);
	/* Look through all the devices for the given bus. */
	therms = ecore_file_ls(busdir);
	if (therms)
	  {
	     char *name;
	     
	     while ((name = ecore_list_next(therms)))
	       {
		  Ecore_List *files;
		  
		  /* Search each device for temp*_input, these should be 
		   * temperature devices. */
		  snprintf(path, sizeof(path),
			   "%s/%s", busdir, name);
		  files = ecore_file_ls(path);
		  if (files)
		    {
		       char *file;
		       
		       while ((file = ecore_list_next(files)))
			 {
			    if ((!strncmp("temp", file, 4)) && 
				(!strcmp("_input", &file[strlen(file) - 6])))
			      {
				 char *f;
				 
				 snprintf(path, sizeof(path),
					  "%s/%s/%s", busdir, name, file);
				 f = strdup(path);
				 if (f) ecore_list_append(result, f);
			      }
			 }
		       ecore_list_destroy(files);
		    }
	       }
	     ecore_list_destroy(therms);
	  }
	ecore_list_first_goto(result);
     }
   return result;
}

static void
init(void)
{
   Ecore_List *therms;
   char        path[PATH_MAX];
#ifdef __FreeBSD__
   int         len;
#endif
   
   if ((!sensor_type) || ((!sensor_name) || (sensor_name[0] == 0)))
     {
	if (sensor_name) free(sensor_name);
	if (sensor_path) free(sensor_path);
	sensor_path = NULL;
#ifdef __FreeBSD__
	/* TODO: FreeBSD can also have more temperature sensors! */
	sensor_type = SENSOR_TYPE_FREEBSD;
	sensor_name = strdup("tz0");
#else
	therms = ecore_file_ls("/proc/acpi/thermal_zone");
	if ((therms) && (!ecore_list_empty_is(therms)))
	  {
	     char *name;
	     
	     name = ecore_list_next(therms);
	     sensor_type = SENSOR_TYPE_LINUX_ACPI;
	     sensor_name = strdup(name);
	     ecore_list_destroy(therms);
	  }
	else
	  {
	     if (therms) ecore_list_destroy(therms);
	     if (ecore_file_exists("/proc/omnibook/temperature"))
	       {
		  sensor_type = SENSOR_TYPE_OMNIBOOK;
		  sensor_name = strdup("dummy");
	       }
	     else if (ecore_file_exists("/sys/devices/temperatures/sensor1_temperature"))
	       {
		  sensor_type = SENSOR_TYPE_LINUX_PBOOK;
		  sensor_name = strdup("dummy");
	       }
	     else if (ecore_file_exists("/sys/devices/temperatures/cpu_temperature"))
	       {
		  sensor_type = SENSOR_TYPE_LINUX_MACMINI;
		  sensor_name = strdup("dummy");
	       }
	     else if (ecore_file_exists("/sys/devices/platform/coretemp.0/temp1_input"))
	       {
		  sensor_type = SENSOR_TYPE_LINUX_INTELCORETEMP;
		  sensor_name = strdup("dummy");
	       }
	     else
	       {
		  // try the i2c bus
		  therms = temperature_get_bus_files("i2c");
		  if (therms)
		    {
		       char *name;
		       
		       if ((name = ecore_list_next(therms)))
			 {
			    if (ecore_file_exists(name))
			      {
				 int len;
				 
				 snprintf(path, sizeof(path),
					  "%s", ecore_file_file_get(name));
				 len = strlen(path);
				 if (len > 6) path[len - 6] = '\0';
				 sensor_type = SENSOR_TYPE_LINUX_I2C;
				 sensor_path = strdup(name);
				 sensor_name = strdup(path);
				 printf("sensor type = i2c\n"
				       "sensor path = %s\n"
				       "sensor name = %s\n",
				       sensor_path, sensor_name);
			      }
			 }
		       ecore_list_destroy(therms);
		    }
		  if (!sensor_path)
		    {
		       // try the pci bus
		       therms = temperature_get_bus_files("pci");
		       if (therms)
			 {
			    char *name;

			    if ((name = ecore_list_next(therms)))
			      {
				 if (ecore_file_exists(name))
				   {
				      int len;

				      snprintf(path, sizeof(path),
					    "%s", ecore_file_file_get(name));
				      len = strlen(path);
				      if (len > 6) path[len - 6] = '\0';
				      sensor_type = SENSOR_TYPE_LINUX_PCI;
				      sensor_path = strdup(name);
				      sensor_name = strdup(path);
				      printf("sensor type = pci\n"
					    "sensor path = %s\n"
					    "sensor name = %s\n",
					    sensor_path, sensor_name);
				   }
			      }
			    ecore_list_destroy(therms);
			 }
		    }
	       }
	  }
#endif
     }
   if ((sensor_type) && (sensor_name) && (!sensor_path))
     {
	switch (sensor_type)
	  {
	   case SENSOR_TYPE_NONE:
	     break;
	   case SENSOR_TYPE_FREEBSD:
#ifdef __FreeBSD__
	     snprintf(path, sizeof(path), "hw.acpi.thermal.%s.temperature",
		      sensor_name);
	     sensor_path = strdup(path);
	     len = 5;
	     sysctlnametomib(sensor_path, mib, &len);
#endif
	     break;
	   case SENSOR_TYPE_OMNIBOOK:
	     sensor_path = strdup("/proc/omnibook/temperature");
	     break;
	   case SENSOR_TYPE_LINUX_MACMINI:
	     sensor_path = strdup("/sys/devices/temperatures/cpu_temperature");
	     break;
	   case SENSOR_TYPE_LINUX_PBOOK:
	     sensor_path = strdup("/sys/devices/temperatures/sensor1_temperature");
	     break;
	   case SENSOR_TYPE_LINUX_INTELCORETEMP:
	     sensor_path = strdup("/sys/devices/platform/coretemp.0/temp1_input");
	     break;
	   case SENSOR_TYPE_LINUX_I2C:
	     therms = ecore_file_ls("/sys/bus/i2c/devices");
	     if (therms)
	       {
		  char *name;
		  
		  while ((name = ecore_list_next(therms)))
		    {
		       snprintf(path, sizeof(path),
				"/sys/bus/i2c/devices/%s/%s_input",
				name, sensor_name);
		       if (ecore_file_exists(path))
			 {
			    sensor_path = strdup(path);
			    /* We really only care about the first
			     * one for the default. */
			    break;
			 }
		    }
		  ecore_list_destroy(therms);
	       }
	     break;
	   case SENSOR_TYPE_LINUX_PCI:
	     therms = ecore_file_ls("/sys/bus/pci/devices");
	     if (therms)
	       {
		  char *name;
		  
		  while ((name = ecore_list_next(therms)))
		    {
		       snprintf(path, sizeof(path),
				"/sys/bus/pci/devices/%s/%s_input",
				name, sensor_name);
		       if (ecore_file_exists(path))
			 {
			    sensor_path = strdup(path);
			    /* We really only care about the first
			     * one for the default. */
			    break;
			 }
		    }
		  ecore_list_destroy(therms);
	       }
	     break;
	   case SENSOR_TYPE_LINUX_ACPI:
	     snprintf(path, sizeof(path),
		      "/proc/acpi/thermal_zone/%s/temperature",
		      sensor_name);
	     sensor_path = strdup(path);
	     break;
	  }
     }
}

static int
check(void)
{
   FILE *f;
   int ret = 0;
   int temp = 0;
   char buf[PATH_MAX];
#ifdef __FreeBSD__
   int len;
#endif
              
   /* TODO: Make standard parser. Seems to be two types of temperature string:
    * - Somename: <temp> C
    * - <temp>
    */
   switch (sensor_type)
     {
      case SENSOR_TYPE_NONE:
	/* TODO: Slow down poller? */
	break;
      case SENSOR_TYPE_FREEBSD:
#ifdef __FreeBSD__
	len = sizeof(temp);
	if (sysctl(mib, 5, &temp, &len, NULL, 0) != -1)
	  {
	     temp = (temp - 2732) / 10;
	     ret = 1;
	  }
	else
	  goto error;
#endif
	break;
      case SENSOR_TYPE_OMNIBOOK:
	f = fopen(sensor_path, "r");
	if (f)
	  {
	     char dummy[4096];
	     
	     fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
	     if (sscanf(buf, "%s %s %i", dummy, dummy, &temp) == 3)
	       ret = 1;
	     else
	       goto error;
	     fclose(f);
	  }
	else
	  goto error;
	break;
      case SENSOR_TYPE_LINUX_MACMINI:
      case SENSOR_TYPE_LINUX_PBOOK:
	f = fopen(sensor_path, "rb");
	if (f)
	  {
	     fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
	     if (sscanf(buf, "%i", &temp) == 1)
	       ret = 1;
	     else
	       goto error;
	     fclose(f);
	  }
	else
	  goto error;
	break;
      case SENSOR_TYPE_LINUX_INTELCORETEMP:
      case SENSOR_TYPE_LINUX_I2C:
	f = fopen(sensor_path, "r");
	if (f)
	  {
	     fgets(buf, sizeof(buf), f);
	     buf[sizeof(buf) - 1] = 0;
	     
	     /* actually read the temp */
	     if (sscanf(buf, "%i", &temp) == 1)
	       ret = 1;
	     else
	       goto error;
	     /* Hack for temp */
	     temp = temp / 1000;
	     fclose(f);
	  }
	else
	  goto error;
	break;
      case SENSOR_TYPE_LINUX_PCI:
	f = fopen(sensor_path, "r");
	if (f)
	  {
	     fgets(buf, sizeof(buf), f);
	     buf[sizeof(buf) - 1] = 0;
	     
	     /* actually read the temp */
	     if (sscanf(buf, "%i", &temp) == 1)
	       ret = 1;
	     else
	       goto error;
	     /* Hack for temp */
	     temp = temp / 1000;
	     fclose(f);
	  }
	else
	  goto error;
	break;
      case SENSOR_TYPE_LINUX_ACPI:
	f = fopen(sensor_path, "r");
	if (f)
	  {
	     char *p, *q;
	     fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
	     fclose(f);
	     p = strchr(buf, ':');
	     if (p)
	       {
		  p++;
		  while (*p == ' ') p++;
		  q = strchr(p, ' ');
		  if (q) *q = 0;
		  temp = atoi(p);
		  ret = 1;
	       }
	     else
	       goto error;
	  }
	else
	  goto error;
	break;
     }
 
   if (ret) return temp;
   
   return -999;
   error:
   sensor_type = SENSOR_TYPE_NONE;
   if (sensor_name) free(sensor_name);
   sensor_name = NULL;
   if (sensor_path) free(sensor_path);
   sensor_path = NULL;
   return -999;
}

static int
poll_cb(void *data)
{
   int t;
   int pp;
   
   t = check();
   pp = cur_poll_interval;
   if (t != ptemp)
     {
	cur_poll_interval /= 2;
	if (cur_poll_interval < 4) cur_poll_interval = 4;
     }
   else
     {
	cur_poll_interval *= 2;
	if (cur_poll_interval > 256) cur_poll_interval = 256;
     }
   /* adapt polling based on if temp changes - every time it changes,
    * halve the time between polls, otherwise double it, between 4 and
    * 256 ticks */
   if (pp != cur_poll_interval)
     {
	if (poller) ecore_poller_del(poller);
	poller = ecore_poller_add(ECORE_POLLER_CORE, cur_poll_interval, 
				  poll_cb, NULL);
     }
   if (t != ptemp)
     {
	if (t == -999) printf("ERROR\n");
	else printf("%i\n", t);
	fflush(stdout);
     }
   ptemp = t;
   return 1;
}

int
main(int argc, char *argv[])
{
   if (argc != 4)
     {
	printf("ARGS INCORRECT!\n");
	return 0;
     }
   sensor_type = atoi(argv[1]);
   sensor_name = strdup(argv[2]);
   if (!strcmp(sensor_name, "-null-"))
     {
	free(sensor_name);
	sensor_name = NULL;	
     }
   poll_interval = atoi(argv[3]);
   cur_poll_interval = poll_interval;
   
   ecore_init();
   
   init();

   if (poller) ecore_poller_del(poller);
   poller = ecore_poller_add(ECORE_POLLER_CORE, cur_poll_interval, 
			     poll_cb, NULL);
   poll_cb(NULL);
   
   ecore_main_loop_begin();
   ecore_shutdown();
   
   return 0;
}
