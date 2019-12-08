#!/bin/bash
undervoltage=-140
last_frequency_tested="0.3GHz"
very_last_frequency="2.4GHz"

if [ -f "keeping_tab/last_frequency_tested" ];
then
	last_frequency_tested=`cat keeping_tab/last_frequency_tested`
	echo >> keeping_tab/fault_results
	echo "******************************************" >> keeping_tab/fault_results 
	echo "Restarting. Assume recovered from crash" >> keeping_tab/fault_results 
fi
echo "last frequency tested: $last_frequency_tested"


if [[ $last_frequency_tested == $very_last_frequency ]];
then
	echo "You're all done!"
	rm keeping_tab/last_frequency_tested
	exit
fi
freq=`grep -A1 "^$last_frequency_tested" keeping_tab/frequencies.txt | grep -v "^$last_frequency_tested"`
echo "New frequency: $freq"
echo "New frequency: $freq" >> keeping_tab/fault_results
echo "$freq" > keeping_tab/last_frequency_tested

sudo cpupower -c all frequency-set -u $freq
sudo cpupower -c all frequency-set -d $freq
# Repeated because sometimes it doesn't work
sudo cpupower -c all frequency-set -u $freq
sudo cpupower -c all frequency-set -d $freq
while true
do
	sleep 1
	undervoltage=$((undervoltage-1))
	val=0x`sudo rdmsr 0x198`
	new_val=$(((val & 0xFFFF00000000) >> 32))
	floating_val=`echo  "scale=3; $new_val/8192" | bc`
	printf "undervolting $undervoltage \t voltage: $floating_val\n" >> keeping_tab/fault_results
	sync
	sudo ~/undervolting/undervolt $undervoltage
done  
