#!/bin/bash
source /home/kit/simple_curses/simple_curses.sh
#    .---------- constant part!
#    vvvv vvvv-- the code from above
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

#Black        0;30     Dark Gray     1;30
#Red          0;31     Light Red     1;31
#Green        0;32     Light Green   1;32
#Brown/Orange 0;33     Yellow        1;33
#Blue         0;34     Light Blue    1;34
#Purple       0;35     Light Purple  1;35
#Cyan         0;36     Light Cyan    1;36
#Light Gray   0;37     White         1;37

clear
cat result_1.txt | grep cipher | cut -f3 -d' ' | tr '\r\n' ' ' > pair1.csv
cat result_2.txt | grep cipher | cut -f3 -d' ' | tr '\r\n' ' ' > pair2.csv

#printf "${RED}"
#printf "**************************************************************\n"
#printf "**                   EXTRACTING THE KEY                     **\n"
#printf "**************************************************************\n"
#sleep 1
printf "${GREEN}"
printf "\nCorrect and faulty pair 1 (pair1.csv)\n"
echo     "-------------------------------------"
printf "${NC}"
cat pair1.csv

printf "${GREEN}"
printf "\n"
printf "\nCorrect and faulty pair 2 (pair2.csv)\n"
echo   	 "-------------------------------------"
printf "${NC}"
cat pair2.csv

printf "${GREEN}"
printf "\n"
printf "\nDifferential fault analysis\n"
echo     "---------------------------"
sleep 1
printf "${NC}"
printf "$ ./dfa 4 1 pair1.csv\n\n"
dfa-aes-master/analysis/dfa 4 1 pair1.csv
cat keys-0.csv|sort >  pair1_keys.csv

printf "\n$ ./dfa 4 1 pair2.csv\n\n"
dfa-aes-master/analysis/dfa 4 1 pair2.csv
cat keys-0.csv|sort >  pair2_keys.csv
sleep 0.5

clear
printf "${GREEN}"
printf "\n\nFinding keys in both files\n"
echo   "--------------------------"
sleep 0.25
printf "${NC}"
printf "$ comm -1 -2 pair1_keys.csv pair2_keys.csv\n"
printf "${YELLOW}\n"
key=`comm -1 -2 pair1_keys.csv pair2_keys.csv`
printf "******************************************************\n"
printf "**       key: $key      **\n"
printf "******************************************************\n"
printf "\n ${NC}"
