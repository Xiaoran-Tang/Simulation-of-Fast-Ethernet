# Simulation-of-Fast-Ethernet
This project is to simulate the Fast Ethernet. The goal of the project is to practice basic socket API programming through a client/server application. It is a simulation in the sense that the fast Ethernet is simulated by multiple processes on multiple machines. Each station in the Ethernet is simulated by a process. The switch of the fast Ethernet is also simulated by a process. Those processes can run on different workstations in the Linux lab.

QUICK AND DIRTY
===============

Execute the following from the proj_Tang/ directory:

    ./configure

    cd lib         # build the basic library that all programs need
    make           # use "gmake" everywhere on BSD/OS systems

    cd ../libfree  # continue building the basic library
    make

    cd ../fast_eth # build and test a basic client program
    make
    
    ./csp > CSP_log.txt
    ./sp 127.0.0.1 9 > SP9_log.txt
    ./sp 127.0.0.1 7 > SP7_log.txt

If all that works, you're all set to test my Fast Ethernet program.
Sample inputs are shown in proj_Tang/fast_eth/input_test#.txt
