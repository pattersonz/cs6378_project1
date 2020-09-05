#!/bin/bash

# Change this to your netid
netid=zjp170130

# Root directory of your project
PROJDIR=/people/cs/s/zjp170130/cs6378_project1

# Directory where the config file is located on your local system
CONFIGLOCAL=config.txt

# Directory your java classes are in
BINDIR=$PROJDIR/bin/unit/

# Your main project class
PROG=unit

n=0

cat $CONFIGLOCAL | sed -e "s/#.*//" | sed -e "/^\s*$/d" |
(
    read i
    echo $i
    while [[ $n -lt $i ]]
    do
    	read line
    	p=$( echo $line | awk '{ print $1 }' )
        host=$( echo $line | awk '{ print $2 }' )
	
	gnome-terminal -e "ssh -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no $netid@$host $BINDIR$PROG; exec bash" &

        n=$(( n + 1 ))
    done
)
