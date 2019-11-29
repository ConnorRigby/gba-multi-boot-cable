/*  Main front-end code for Matt'sSerialMultibootCable
    Portions derived from AJO's dumb parallel port cable code.

    Rest (c) 15th May 2002 Matt Evans
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int verbose = 1;
int verbose2 = 0;
int opendelay = 0;

char *device = "/dev/ttyS0";
extern void xfer_init(void);
extern void xfer_passthru(void);
extern int multi(int);       /* The multiboot code to do the transfer encoding */

//unsigned char dout=255;

void ProgramExit(int n)
{
	exit(n);
}

/* Code for the 2ndstage bootstrap  here: */

#include "2ndloader/loader.h"

unsigned char Client[256*1024];
int ClientLength;

void usage(char *name)
{
  fprintf(stderr, "[GBA serial multiboot loader v1.01]\nUsage: %s [-v] [-an] [-p<path>] file.mb\n"
                  "\t-v:\t\tVerbose (serial link debugging)\n"
                  "\t-dn:\t\twait for n (0 - 9) seconds after opening the serial port\n"
                  "\t\t\tRequired on arduinos which auto reset\n"
                  "\t-p<path>:\tPath to the device the multiboot cable is connected to\n"
                  "\t\t\tDefaults to -p/dev/ttyS0\n"
                  , name); 
}

/***********************************************************************
  Main code:  Load a file, send the loader, drop the cable down into 
  'passthru' UART mode, and download the file into the GBA.
 ***********************************************************************/

int main(int argc,char **argv)
{
  int i;
    
  /* Check that arguments are correct: */
  if (argc < 2) 
    {
      usage(argv[0]);
      exit(0);
    }
  
  for (i = 1; i <argc; i++) 
    {
      if (argv[i][0] != '-') 
	break;
      if (argv[i][1] == 'v')
	verbose2 = 1;
      else if (argv[i][1] == 'd')
      	{
       	  opendelay = atoi(&argv[i][2]);
       	  if (opendelay < 0 || opendelay > 9)
       	    {
              fprintf(stderr,"No valid time specified after -d (note there must be no space after -d)\n");
              exit(1);
       	    }
      	}
      else if (argv[i][1] == 'p')
        {
          device = &argv[i][2];
          if (strlen(device) <1)
            {
              fprintf(stderr,"No path specified after -p (note there must be no space after -p)\n");
              exit(1);
            }
        }
      else if (argv[i][1] == 'h')
	{
	  usage(argv[0]);
	  exit(0);
	}
      else 
	{
	  fprintf(stderr,"Duplicate or unknown flag.\n");
	  exit(1);
	}
    }
  
  if (i == argc) 
    {
      fprintf(stderr,"Missing input file.\n");
      exit(1);
    }
  
  
  xfer_init();
  
  memcpy(&Client, loader, loaderlen);
  ClientLength = loaderlen; 
  
  printf("-> Sending 2ndstage loader (%d bytes):\n", ClientLength);
  
  verbose = 1;
  
  if (multi(1))  /* Send the data with multi() */
    {
      printf("[ Transmission error! :( Giving up. ]\n");
      exit(0);
    }
  else
    {
      /* All ok, so talk to the monitor loader on the GBA */
      xfer_passthru();
      xfer_file(argv[i]);
      
    }
  
  return 0;
}
