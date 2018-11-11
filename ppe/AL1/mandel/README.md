
#Compilation

	For compiling all the program, there is a Makefile. You only need to run the following command

	$ make

#Re-Compilation

	For re-compiling your program you just need to clean your old files and compile again with the following sequence of commands:

	$ make cleanall

	$ make

#Display informations
	
	when you want to run experiments, you can disable the display by setting the following environment variable

	$ export MANDEL_NO_DISPLAY=1

	and then compile again you program with

	$ make

	*When you want run again with display, please unset your environment variable by using the following command*

	$ unset MANDEL_NO_DISPLAY

#Execution
	
	Parameters:	
	$ ./seq [size of the image] [number of iterations]


	Command example:
	$ ./seq 1000 20000