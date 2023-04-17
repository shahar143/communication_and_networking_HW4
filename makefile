makefile:

all: ping better_ping watchdog

ping : ping.c
	gcc -o ping ping.c

better_ping : better_ping.c
	gcc -o better_ping better_ping.c

watchdog: watchdog.c
	gcc watchdog.c -o watchdog

clean:
	rm -f *.o myping watchdog better_ping