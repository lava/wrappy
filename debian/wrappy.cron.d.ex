#
# Regular cron jobs for the wrappy package
#
0 4	* * *	root	[ -x /usr/bin/wrappy_maintenance ] && /usr/bin/wrappy_maintenance
