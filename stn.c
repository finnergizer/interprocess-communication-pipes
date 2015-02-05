/*------------------------------------------------------------
File: stn.c

Description: This program implements the station process that
sends messages to another station process.  The content of the 
configuration file determines the station identifer (first data line)
and the identifier of the station to which messages are sent (second
data line).  Other data lines in the configuration files are
messages to be sent (lines starting with # or empty are ignored).

After each message is sent the station process waits for an 
acknowledgement (Ack message).  All communication is done using
the standard input and standard output.  The station process can still
print to the screen using the standard error. When the station process
receives a messages, it reponds by returning an acknowledgement.
-------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// Some definitions
#define MSGS_MAX 10 // Maximum number of messages
#define TRUE 1
#define FALSE 0

// for messages
#define ACKNOWLEDGEMENT "Ack"  // acknowledgement message
#define STX '@'       // Start of the frame - start of Xmission
#define ETX '~'       // End of the frame - end of Xmission
#define STX_POS 0     // Position of STX
#define DEST_POS 1    // Position of the destination identifier
#define SRC_POS 2     // Position of the source identifier
#define MSG_POS 4     // Position of the message

// Return values
#define FINISH 1
#define MSG_ACK 2
#define MSG_EMPTY 3
#define MSG_RECV 4

#define BUFSIZE 100
// Prototypes
void readFile(FILE *, char *, char *, char *[], char *);
void communication(char, char, char *[]);
int readMessage(char *, char *, char );
int extractMessage(char *, char *, char *, char );

/*-------------------------------------------------------------
Function: main
Parameters: 
    int ac - number of arguments on the command line
    char **av - array of pointers to the arguments
Description:
   Evaluates the command line arguments and opens the configuration
   file.  The function readFile() configures the station/destination 
   identifiers and reads in the messages.  If no error is found in the
   configuration file, communication() is called to exchange messages
   with the other station processes.
-------------------------------------------------------------*/
int main(int ac, char **av)
{
   char dest;                   // destination identifier
   char idStn;                  // station identificatier
   char *messages[MSGS_MAX+1];  // array of pointers to messages - terminated with NULL
   char msgsBuffer[BUFSIZ];     // buffer of messages
   FILE *fp;
   
   
   if(ac != 2)
   {
       fprintf(stderr,"Usage: stn <fileName>\n");
   }
   else
   {
      fp = fopen(av[1],"r");
      if(fp == NULL) 
      {
        sprintf(msgsBuffer,"stn (%s)",av[1]);
        perror(msgsBuffer);
      }
      else
      {
         readFile(fp, &idStn, &dest, messages, msgsBuffer);
	 fclose(fp);
	 if(idStn != '\0' && dest != '\0') 
	   communication(idStn, dest, messages);
	 else fprintf(stderr,"File corrupted\n");
      }
   }
}
/*-------------------------------------------------------------
Function: readFile
Parameters: 
	fp	 - file pointeur
	idStnPt  - pointeur to return station identifier
	destPt	 - pointeur to return destination identifier
	msgs     - array of pointers to messages for transmission
	msgsBuf  - pointer to buffer to store messages
Description:
   Read all lines in the file. All empty lines and those starting with # are ignored.
   First line: use the first character as the station id
   Second line: use the first character as the destination id
   Other lines: are the messages.
   (care must be taken with inserting spaces in the file).
-------------------------------------------------------------*/
void readFile(FILE *fp, char *idStnPt, char *destPt, char *msgs[], char *msgsBuf)
{
    char line[BUFSIZ];   // for reading in a line from the file
    char *pt = msgsBuf;  // pointer to add messages to the message buffer msgsBuf
    int i = 0;           // index into the pointer array

    // Some initialization
    *idStnPt = '\0';  
    *destPt = '\0';
    msgs[i] = NULL;  // empty list
    while(fgets(line, BUFSIZ-1, fp) != NULL)
    {
       if(*line != '\n' && *line != '#' && *line != '\0')  // to ignore lines
       {
           if(*idStnPt == '\0') // found first line
	       *idStnPt = *line;  // get first character in the line
	   else if(*destPt == '\0') // found second line
	       *destPt = *line;  // get first character in the line
	   else // all other lines are messages to be saved
	   {
	      line[strlen(line)-1] = '\0';  // remove the \n at the end of the line
	      if(i < MSGS_MAX && strlen(line)+strlen(msgsBuf)+1 < BUFSIZ) // in order not to exceed limites 
	      {
                  strcpy(pt,line);        // copy messages into the buffer
		  msgs[i++] = pt;         // save its address in the array
		  msgs[i] = NULL;         // end the liste in the array with NULL
		  pt += strlen(line)+1;   // point to next free space in buffer; the +1 is for the '\0'
	      }
	   }
       }
    }
}

/*-------------------------------------------------------------
Function: communication
Parameters: 
	idStn    - station identifier
	dest	 - destination identifier
	messages - array of pointers to messages for transmission
Description:
   In a loop send the messages found in the array "messages". Between
   the transmission of each message wait for an acknowledgement (note
   that ackFlag ensures that an acknowldegement has been received
   before transmitting the next message).
   When a message is received, print to the screen (using standard
   error) the message and send an acknowledgement to the source
   of the message.
   The loop is broken when the standard input is closed (e.g.
   the write end of the pipe is closed) - this is detected by
   readMessage().
   Note that readMessage() blocks when the pipe attached to 
   the standard input is empty.
-------------------------------------------------------------*/
void communication(char idStn, char dest, char *messages[])
{
   int i=0;                // index for messages[]
   int ackFlag = TRUE;     // acknowledgement flag
   int flag;               // return flag from readMessage()
   char source;            // source identificateur for received message/Ack
   char msg[BUFSIZ];       // buffer for received message

   // loop for transmission and reception
   while(1) 
   {
      // Transmission of messages 
      if(ackFlag && (messages[i] != NULL))
      {  // Send message
         fprintf(stderr,"Station %c (%d): Sent to station %c >%s<\n",idStn,getpid(),dest,messages[i]);
         sprintf(msg,"%c%c%c-%s%c",STX,dest,idStn,messages[i],ETX);
         write(1,msg,strlen(msg));   // writes to the standard output, i.e. pipe
         ackFlag = FALSE;            // becomes TRUE at the arrival of an ack
	 i++;                        // points to next message for next time
      }

      // Reception de messages 
      flag = readMessage(msg, &source, idStn); 
      if(flag == MSG_ACK)  // Acknowledgement received
      {
          if(source == dest) // check out the source
          { 
	    ackFlag = TRUE;   
            fprintf(stderr,"Station %c (%d): Received from station %c an acknowledgement\n", idStn, getpid(), source, msg);
          } 
	  else fprintf(stderr, "Station %c (%d): received an Ack from %c - ignored\n",idStn,getpid(),source);
      }
      else if(flag == MSG_RECV) 
      {     // Received a message - msg contains it, source gives id station that sent it
         fprintf(stderr,"Station %c (%d): Received from station %c >%s<\n", idStn, getpid(), source, msg);
         sprintf(msg,"%c%c%c-%s%c",STX,source,idStn,ACKNOWLEDGEMENT,ETX); // envoie acquittement
         write(1,msg,strlen(msg));
      }
      else if(flag == FINISH) break; // comms channel (pipe) was closed
      else // fatal or unknown error
         fprintf(stderr,"Station %c (%d): unknown value returned by readMessage (%d)\n",idStn,getpid(),flag);
   }
}

/*-------------------------------------------------------------
Function: readMessage
Parameters: 
	msg	  - pointer to buffer for storing received message
	sourcePtr - pointer to a character to receive source identifier
Description:
    Reads in a buffer of many frames (or 1) from the standard input (i.e.
    pipe).  If the standard input is closed, return FINISH. Note that the 
    buffer allFrames is static and does not disappear between calls
    to the function.

    If frames have been received, call extractMessage() to extract the
    first message; it returns MSG_ACK if an acknowledgement is found or
    MSG_RECV if a message is found (message shall be copied to the buffer
    referenced by buffer).  Messages are removed from allFrames by 
    extractMessage(). Note that extractMessage ignore frames not addressed
    to this station process (i.e. the destination ident. is not equal to
    idStn).  Once allFrames is empty, fill it again from the standard input.

    Thus this function scans the standard input for messages until it
    finds one destined for station process idStn or until the standard input
    is closed.

    See extractMessage() for frame format.
-------------------------------------------------------------*/
int readMessage(char *msg, char *sourcePtr, char idStn)
{
   static char allFrames[BUFSIZ];  // all frames read from pipe - static buffer
   int ret;			   // value returned by this function
   int retRead;			   // to store value returned by read and extractMessage function
   char msgErreur[BUFSIZ];         // buffer to build error messages
   
   while(1) // Loop to find a message
   {
      if(*allFrames == '\0') // buffer empty - need to read from the pipe
      {
        retRead = read(0,allFrames,BUFSIZ-1); // blocks when pipe is empty
	if(retRead == -1) 
	{
            sprintf(msgErreur,"Station %c (%d): reading error",msgErreur,idStn);
            perror(msgErreur);
	}
	else if(retRead == 0) // write end of pipe has been closed
	{
	    ret = FINISH;
	    break;  // break out of loop
	}
	// messages are in the buffer
	allFrames[retRead]='\0';  // terminate the string
      }
      // The following line can be used for debugging
      //fprintf(stderr,"Station %c (%d): readMessage >%s<\n", idStn, getpid(), allFrames);
      retRead = extractMessage(msg, allFrames, sourcePtr, idStn);
      if(retRead != MSG_EMPTY) // if MSG_EMPTY, no messages were found in the buffer 
      {
          ret = retRead;  // is MSG_ACK or MSG_RECV
	  break;
      }
      // if we get here, need to read from the pipe again
   }
   return(ret);
}

/*------------------------------------------------
Function: extractMessage

Parameters:
    msg		- points to buffer to receive a message
    aframes 	- points to buffer of received frames
    sourcePtr   - to returne the identifier of the source 
    idStn       - this stations identifier

Description: 
     Extracts a message from the buffer referenced by aframes.
     The message is removed and copied to the buffer referenced
     by msg. Frames with improper destination id are skipped.

     Message format: STX D S <message> ETX
     D must be equal to idStn to gain attention
     S gives the ident. of the station that sent the message 
     <message> - string of characters
     If STX is missing, print an error and skip the message.
------------------------------------------------*/
int extractMessage(char *msg, char *aframes, char *sourcePtr, char idStn)
{
   char *pt=aframes;       // pointer to navigate the buffer of all frames
   int retcd = MSG_EMPTY;  // return value 

   while(1) // find a message for this station
   {
      if(*pt == '\0') // no messages
      {
         retcd = MSG_EMPTY;
	 *aframes = '\0';  // empties the all frames buffer
         break; // break the loop
      }
      else if(*pt != STX) // found an error - no STX
      {
	  fprintf(stderr,"stn(%c,%d): no STX: >%s<\n",idStn,getpid(),pt);
          while(*pt != ETX && *pt != STX && *pt != '\0') pt++;  // skip until the end or beginning 
	  if(*pt == ETX) pt++; 					// skip the ETX
      }
      else if(*(pt+DEST_POS) != idStn)  // not my message - ignore
      {
          // The following lines can be used for debugging
          //fprintf(stderr,"stn (%c,%d): Skipping message destined to another destination: >%s<\n",
          //               idStn, getpid(), pt);  
          while(*pt != ETX && *pt != '\0') pt++;  // skip to the end
	  if(*pt == ETX) pt++; 			  // skip the ETX
      }
      else if(strncmp(pt+MSG_POS,ACKNOWLEDGEMENT,strlen(ACKNOWLEDGEMENT)) == 0) // found an ACK message
      {
         *sourcePtr = *(pt+SRC_POS);             // to return the source ident.
         retcd = MSG_ACK;
	 *msg == '\0';
         while(*pt != ETX && *pt != '\0') pt++;  // skip to the end
	 if(*pt == ETX) pt++;                    // skip the ETX
	 strcpy(aframes,pt);                     // move unread frames to the start of the buffer
	 break;
      }
      else // found a message
      {
         *sourcePtr = *(pt+SRC_POS);                       // to return the source ident.
         retcd = MSG_RECV;
	 pt = pt+MSG_POS;                                  // point to the message
         while(*pt != ETX && *pt != '\0') *msg++ = *pt++;  // copy message into buffer
	 if(*pt == ETX) pt++;                              // skip the ETX
	 *msg = '\0';                                      // terminate the string
	 strcpy(aframes,pt);                               // move unread frames to the start of the buffer
	 break;
      }
   }
   return(retcd);
}
