
# ffmpeg Commands

mini personal project that performs ffmpeg commands on all video files inside of a directory

### Dependencies 
Make sure ffmpeg is installed and as well as GCC compiler

Make sure the executable is in the right path relative to video directory, 2 levels below video directive (../../)





![image of correct path for code relative to video directory](./images/hier.PNG)


### Executing
compile using `gcc main.c` to generate a.out executable.
When you execute it, it will prompt u for a directory to start performing ffmpeg commands, simply type in the directory name, do not type in the absolute path, the program assumes to look at `../../directory`

