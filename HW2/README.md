### Samuel Cuthbertson 
Email: Samuel (DOT) Cuthbertson (AT) colorado (DOT) edu

### Content and Relevant Files:
All content assusmes running in the CU-CS-VM Fall 2017 virtual machine. 
```
HW2
+--module
|  +--build.sh (Shell script to build and load module)
|  +--Makefile (Makefile for build.sh)
|  +--simpleCharDriver.c (Source for the driver)
+--client
|  +--client.c (Userspace program for using the driver)
```
#### To Run:
1. Change directory to module, run ``sh ./build.sh``
2. Change directory to client, run ``gcc client.c``
3. Run ``./a.out``
4. Check kernel output with ``dmesg``
