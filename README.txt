 - README - 

=====================	     [Usage notes]		======================

(compile and run):

$ make
$ ./bbbserver [-c] <config file>


(remove object files):

$ make clean


===================	     [Program Files]		=====================

1. Included config file(s) useful info:
	- master1.conf
		name 		= “master1”
		front-end port 	= 9001
		back-end port 	= 9002

	- slave1.conf
		name 		= “slave1”
		front-end port 	= 9010
		back-end port 	= 9002

	- slave2.conf
		name 		= “slave2”
		front-end port 	= 9011
		back-end port 	= 9004

	- master1.conf
		name 		= “slave3”
		front-end port 	= 9012
		back-end port 	= 9002
	


===================	     [External Libraries]	=====================

1. SPCUUID : { https://github.com/spc476/SPCUUID }
	- general use UUID generation library
	- source files: 
		p2/spcuuid/src
	- object files:
		p2/spcuuid/obj

*  Using this because we were having trouble with linker finding and using the default UUID lib on the ECE cluster machines
** Not including “uuidlib_v3.o” and “uuidlib_v5.o” object files due to linker issues 



===================		[Known Issues]		=====================


1. /peer/map Requests not implemented

2. /peer/rank/<content_path> Requests not implemented


===================		   [Other]		=====================

Authors: Malcolm Fitts (mfitts) and Sam Adams (sjadams)

