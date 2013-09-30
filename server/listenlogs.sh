while true; do
	clear
	cat /var/log/syslog | grep MSIU | tail
	sleep 2
done;
