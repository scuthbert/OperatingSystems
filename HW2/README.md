### Samuel Cuthbertson 
Email: Samuel (DOT) Cuthbertson (AT) colorado.edu

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
1. Build hellocall or simpleadd with ``gcc hellocall.c`` or ``gcc simpleadd.c``
2. Run ``./a.out``
3. Check kernel output with ``dmesg``
