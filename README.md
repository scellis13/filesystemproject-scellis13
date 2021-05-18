# The DaemonDemon File System

The DaemonDemon File System is a Console-Based Application built in C. It utilizes San Francisco State University, Professor Robert Bierman's Low-Level File System Driver. The purpose of this Application is to build and navigate through a custom File System structure. Volume Blocks are simulated through the fslow.c file, by writing block bytes to an external file. 

## Installation

A makefile is provided to allow for 'make' commands.

```
#Using make:
make run

#Using gcc:
gcc -c -o fsshell.o fsshell.c -g -I.
gcc -c -o fsLow.o fsLow.c -g -I.
gcc -o fsshell fsshell.o fsLow.o -g -I. -lm -l readline -l pthread
./fsshell TestVolume 1000000 512
```

## Usage

```C
//Type 'help' for a list of commands.
```

## License
[SE Development](https://github.com/scellis13/)
