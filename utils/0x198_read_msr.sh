#!/bin/bash
while [ 1==1 ]
do
    val=0x`sudo rdmsr 0x198`
    new_val=$(((val & 0xFFFF00000000) >> 32))
	floating_val=`echo  "scale=3; $new_val/8192" | bc`
    printf "\nreading msr register 0x198. Voltage: $floating_val" 
    sleep 1  
done
