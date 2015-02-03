all: stn hub

stn: stn.c
	cc -o stn stn.c

hub: hub.c
	cc -o hub hub.c -lpthread
