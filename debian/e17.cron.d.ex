#
# Regular cron jobs for the e17 package
#
0 4	* * *	root	[ -x /usr/bin/e17_maintenance ] && /usr/bin/e17_maintenance
