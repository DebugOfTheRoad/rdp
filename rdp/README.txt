Copyright (c) 2001 - 2011, The Board of Trustees of the University of Illinois.
All Rights Reserved.
Copyright (c) 2011 - 2012, Google, Inc. All Rights Reserved.

UDP-based Reliable Data Transfer (RDP) Library - version 1
Author: houbaoshan 

RDP is free software under BSD License. See ./LICENSE.txt.

============================================================================

BASED ON UDP


CONTENT: 
./src:     source code 
./app:     Example programs 
./doc:     documentation (HTML)
./win:     Visual C++ project files for the Windows version  


To make: 
     make -e os=XXX arch=YYY 

XXX: [LINUX(default), BSD, OSX] 
YYY: [IA32(default), POWERPC, IA64, AMD64] 

For example, on OS X, you may need to do "make -e os=OSX arch=POWERPC"; 
on 32-bit i386 Linux system, simply use "make".

On Windows systems, use the Visual C++ project files in ./win directory.

Note for BSD users, please use GNU Make.

To use RDP in your application:
Read index.htm in ./doc. The documentation is in HTML format and requires your
browser to support JavaScript.

git:
https://github.com/iwallee/rdp
