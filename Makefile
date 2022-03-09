CC = gcc 
CFLAGS = -Wall -pedantic -std=gnu99 -I/local/courses/csse2310/include -L/local/courses/csse2310/lib -lcsse2310a3 -g 
 
DEBUG = -g 
 
.PHONY: all debug  
.DEFAULT_GOAL := all 

debug: CFLAGS += $(DEBUG) 
debug: clean $(TARGETS) 

all: jobrunner 

#link jobrunner from object files 
jobrunner: jobrunner.o run.o 
	$(CC) $(CFLAGS) jobrunner.o run.o -o jobrunner

#compile source files to objects 
jobrunner.o: jobrunner.c jobrunner.h 
	$(CC) $(CFLAGS) -c jobrunner.c

run.o: run.c run.h 
	$(CC) $(CFLAGS) -c run.c 

clean: 
	rm -f jobrunner *.o  

