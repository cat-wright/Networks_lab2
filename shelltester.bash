#!/bin/bash

#NOTE: If you do NOT want your computer to open 10,000 web browsers and print 20,000 raw bytes to stdout, it is highly suggested that you edit 
#the .c files, turn the TESTING and DEBUG constants to true and false, respectively, and re-make the whole thing.

#usage: ./shelltester.bash IP num_runs TCP/UDP

for i in `seq 1 $2`
do
./client http://$1:8080/lab2.html $3 & 2> /dev/null
done
