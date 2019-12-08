#!/bin/bash
if [ $# -ne 1 ] ; then
	echo "Incorrect number of arguments" >&2
	echo "Useage $0 <frequency>" >&2
	echo "Example $0 1.6GHz" >&2
    exit
fi
sudo cpupower -c all frequency-set -u $1
sudo cpupower -c all frequency-set -d $1

echo "sleeping to take effect.."
sleep 1
grep "cpu MHz" /proc/cpuinfo