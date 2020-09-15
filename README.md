#What this is
This program opens up series of processes on the UTDallas distributed computers, with a topography specified in the config.txt. From here, a main process(the origin) will communicate the topography and ports to each process where they will communicate to specify their eccentricity. Once a process has found all N nodes, it responds to the origin with all of it's information. Once all processes have responded the total results will be output and the origin program terminates.

##To Run
First the program must be built on the origin system as well as one of the distributed computers.

###To Build
in the root directory:
mkdir build
cd build
cmake ..
make

Once the program is built, the system can be executed with the following
bash launcher.sh  #ctrl+c can be used to terminate this once all windows have opened
./build/origin/origin
bash cleanup.sh #this will shutdown the hanging processes on the distributed computers.

note: if you run "bash launcher.sh gdb" then each process will launch in a gdb shell for debugging.