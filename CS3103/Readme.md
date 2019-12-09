# CS3103 Assignment

For Assignment Instructions, see `Assignment 1.pdf`

All files were tested in an Linux Ubuntu 16.04 environment

## Part B instructions

Files: `PartB.cpp`

Just compile with g++ and run the resulting executable. No command line input is required (port and host ip are already specified)

## Part C instructions

Files: `PartC.cpp`

The program takes in two command line arguments:

1. The private IP address of the client computer running this program. It has been tested on the wlp2s0 interface on NUS network.
2. The host URL

Sudo priveillages are required to run this program.

Hence, when running the program, please run it in the following way: `sudo <program name> <source private IP address> <host url>`, for example: `sudo A0164750E_PartC.cpp 192.168.1.247 luminus.nus.edu.sg`

The program is designed to terminate after a maximum of 30 hops

## Part D instructions

Files: `PartD.cpp`, `compilePartD.sh`

To compile the cpp file, execute the `compilePartD.sh` file (i.e. `./compilePartD.sh`).

Alternatively, run the following command: `g++ PartD.cpp -std=c++11 -l curl -o PartD.out`

The list of initial urls are inside the cpp file, under the main function. They are stored in a variable called `initial_urls`. To add/remove urls from the initial url list, modify the `initial_urls` accordingly and recompile the cpp file.

Result will be written to a text file called `urls.txt`

## Miscellaneous Files

All other c/cpp files (e.g. `tcpclient.c`) were provided as examples by the course instuctor
