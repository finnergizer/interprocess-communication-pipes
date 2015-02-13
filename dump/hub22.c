/*------------------------------------------------------------
File: hub.c   (CSI3131 Assignment 1)

Student Name: Shaughn Finnerty

Student Number: 6300433

Description:  This program creates the station processes
     (A, B, C, and D) and then acts as a Ethernet/802.3 hub.
-------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define OK 1
#define PROGRAM_STN "stn"  // The program that acts like a station
#define MAX_STNS 10        // Maximum number of stations
// Note that the terms reception and transmission are relatif to the station and not the hub
// Note that the descriptors at the same index in the two arrays are related to the same station,
// for example, fdsRec[2] and fdsTran[2] contain the fds of the pipes connected to the same station.
int fdsRec[MAX_STNS+1];    // file descriptors for writing ends (reception)
int fdsTran[MAX_STNS+1];   // file descriptors for reading ends (transmission)

/* Prototypes */
void createStation(char *);
void createHubThreads();
void *listenTran(void *);

/* Temporary */
int pipeCounter = 0;

/*-------------------------------------------------------------
Function: main
Parameters:
    int ac - number of arguments on the command line
    char **av - array of pointers to the arguments
Description:
    Creates the stations using createStation() and threads using
    using createHubThreads().  createHubThreads() cancels
    (terminates) the threads after 30 seconds.
-------------------------------------------------------------*/
int main(int ac, char **av)
{
   int i;

   // Initialization
   fdsRec[0] = -1; // empty list
   fdsTran[0] = -1; // empty list
   
   // Creating the stations
   createStation("/home/genh/h/f8/sfinn038/School/CSI3131/a1/stnA.cfg");
   createStation("/home/genh/h/f8/sfinn038/School/CSI3131/a1/stnB.cfg");
   createStation("/home/genh/h/f8/sfinn038/School/CSI3131/a1/stnC.cfg");
   createStation("/home/genh/h/f8/sfinn038/School/CSI3131/a1/stnD.cfg");
   sleep(200);
   // creating threads for the hub
   createHubThreads();  
   // On return from the function - all threads are terminated.
   // When the hub process terminates, all write ends of the reception pipes
   // are closed, which should have the stations terminate.
   return(0);  // All is done.
}

/*-------------------------------------------------------------
Function: createStation
Parameters:
    fileConfig - refers to the name of the configuration file
Description:
    Creates a station process (stn) which acts like a station
    according to the content of the configuration file (see stn.c).
    Must create 2 pipes and organize them as follows:
    Transmission pipe:  The write end is attached to the standard
                        output of the station process (stn).
			The read end remains attached to the hub
			process and its file descriptor is stored
			in fdsTran (at its end).
    Reception  pipe:  The read end is attached to the standard
                      input of the station process (stn).
		      The write end remains attached to the hub
		      process and its file descriptor is stored
		      in fdsRec (at its end).
    Note that the fds of the pipes in the fdsTran and fdsRec arrays
    are stored at the same index.
    All fds not used in both the station and hub processes are closed.
-------------------------------------------------------------*/
void createStation(char *fileConfig)
{
	
        /* To be completed - create pipes and station process */
	int pid;
	int tranPipeFds[2];
	int recPipeFds[2];
	int pipeStatus;
	
	pipeStatus = pipe(tranPipeFds);
	if (pipeStatus == -1){
		fprintf(stderr, "Transmission pipe creation failed.");
		exit(-1);
	}
	pipeStatus = pipe(recPipeFds);
	if (pipeStatus == -1){
		fprintf(stderr, "Reception pipe creation failed.");
		exit(-1);
	}
	
    pid = fork(); /* fork another process */
    if (pid < 0){
        fprintf(stderr, "Fork Failed");
        exit(-1); /* fork failed */
    } else if (pid == 0) { /* child process */
		//attach std out,1, fds to write end [1] of tran pipe, close read end fds to tran pip
		//dup close stdout fds to parent process
		dup2(tranPipeFds[1],1);
		close(tranPipeFds[0]);
		close(tranPipeFds[1]);
		//attach std in,0, fds to read end of [0] rec pipe, close write end fds to rec pipe
		//dup closes stdin fds to parent proces
		dup2(recPipeFds[0], 0);
		close(recPipeFds[1]);
		close(recPipeFds[0]);
		//recPipeFds[0]=-1;
		
		int c;
		for (c=0; c<pipeCounter; c++){
			close(fdsRec[c]);
			close(fdsTran[c]);
		}
		int i = execlp("/home/genh/h/f8/sfinn038/School/CSI3131/a1/stn", PROGRAM_STN, fileConfig, NULL);
		//i will only be assigned a value if this call fails (i.e. -1)
		printf("%d\n", i);
    } else {
		//store read end of tran pipe [0] in fdsTran(at its end)
		//fprintf(stderr, "%d,%d\n",tranPipeFds[0], tranPipeFds[1]);
		//fprintf(stderr, "%d,%d\n",recPipeFds[0], recPipeFds[1]);
		//dup2(fdsTran[pipeCounter], tranPipeFds[0]);
		fdsTran[pipeCounter] = tranPipeFds[0];
		//IS THIS NECESSARY???
		close(tranPipeFds[1]);
		//close(tranPipeFds[0]);
		//store write end of rec pipe [1] in fdsRec(at its end)
		fdsRec[pipeCounter] = recPipeFds[1];
		//dup2(fdsRec[pipeCounter], recPipeFds[1]);
		//IS THIS NECESSARY???
		close(recPipeFds[0]);
		//close(recPipeFds[1]);
	
		pipeCounter = pipeCounter + 1;
		fdsRec[pipeCounter] = -1;
		fdsTran[pipeCounter] = -1;	
		//fprintf(stderr, "Initialized pipes for stn %d\n", pipeCounter);
		sleep(1);	
	// wait(NULL);
		//Debugging

		

    }

}

/*--------------------------------------------------------------
Function: createHubThreads
Description:
   Create 4 threads to listen on each T-pair pipe (i.e to the
   fd's in dfsTran) with the function listenTran.
   Once the threads have been created, sleep for 30 seconds and
   then cancel (terminate) the threads.
--------------------------------------------------------------*/
void createHubThreads()
{

        /* To be completed - create the four threads  */
	pthread_t tid[pipeCounter];
	pthread_attr_t attr;
	int i;

	//pthread_init_attr(&attr);
	for (i = 0; i<pipeCounter; i++){
		// pthread_create(&tid[i], &attr, listenTran, &fdsTran[i]);
		pthread_create(&tid[i], NULL, listenTran, &fdsTran[i]);
	}
	// Sleep a bit
	sleep(30);
	for (i=0; i<pipeCounter; i++){
		pthread_cancel(tid[i]);
	}

	// Cancel the threads

	    /* To be completed - have the four threads terminate */
}

/*-------------------------------------------------------------------
Function: listenTran

Description: 
   This function runs in a thread an listens to a station process
   on a transmission pipe (station transmits).  When data is read
   from the pipe, it is copied into all reception pipes except for
   the one attached to the process that sent the data.
-------------------------------------------------------------------*/
void *listenTran(void *fdListenPtr)
{
   int fdListen = *((int *) fdListenPtr);  // Get the fd on which to listen
   int num;                                // value returned by read (num of bytes read)
   char buffer[BUFSIZ];                    // buffer for reading data
   int i;
   //sleep(200); 
   while(1)  // a loop
   {
     num = read(fdListen,buffer,BUFSIZ-1);
     if(num == -1) // error in reading 
     {
        sprintf(buffer,"Fatal error in reading on fd %d (%d)",fdListen,getpid());
	perror(buffer);  			/* writes on standard error */
	break;  				/* break the loop */
     }
     else if(num == 0) /* other end of pipe closed - should not happen */
     {
	sprintf(buffer,"Pipe closed (%d)\n",getpid());
        write(2,buffer,strlen(buffer)); 	// write to standard error
	break;  				/* break the loop */
     }
     else // ecrite dans le tuyaux de reception
     {
       buffer[num] = '\0';  			// terminate the string
       // Following line can be used for debugging
       //printf("hub: received frame from %d >%s<\n",fdListen,buffer);
       for(i=0 ; fdsRec[i] != -1 ; i++)
          if(fdsTran[i]!=fdListen) 
          {
               // Following line can be used for debugging
               //printf("hub: transmitting frame to %d >%s<\n",fdsRec[i],buffer);
	       write(fdsRec[i],buffer,strlen(buffer));
          }
     }
   }
}
