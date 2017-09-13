### Samuel Cuthbertson 
Email: Samuel (DOT) Cuthbertson (AT) colorado.edu

### Content and Relevant Files:
Assuming linux kernel (4.10) is set up at ``/home/kernel/linux-hwe-4.10.0``, all file structures are relative to that folder.
```
linux-hwe-4.10.0
+--arch
|  +--x86
|  |  +--kernel
|  |     +--simple_add.c (Source for sys_simple_add, system call which adds two number)
|  |     +--Makefile (Modified Makefile with helloworld and simple_add)
|  +--entry
|     +--syscalls
|        +--syscall_64.tbl (Modified syscall tbl with helloworld and simple_add)
+--include
   +--linux
      +--syscalls.h (Modified syscalls list with helloworld and simple_add)

var
+--log
   +--syslog (Example outputs from hellocall.c and addcall.c)

hellocall.c (C program for calling sys_hello_world)
addcall.c (C program for calling sys_simple_add) 
```
