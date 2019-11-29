/* Code to transfer an IWRAM image to the GBA via
   a plain serial connection, communicating with 2ndloader 
   monitor code in the GBA.  (The image is not NECESSARILY 
   a multiboot image, since the loader will download up to 256K into
   IWRAM and then simply branch to the start of it.  This will work
   for MB images, but you can leave out the header if you want to 
   save 50 bytes or something. ;-D  

   Part of Matt'sSerialMultibootCable, (c) Matt Evans, 15th May 2002
   Must be distributed with the complete tarball.  This code is released
   under the GPL.
*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>


#define XFERBLOCK 512

int ser_fd;
FILE *ser_fp;

extern unsigned char Header [];
extern int opendelay;
extern char *device;

void serial_error(char *s)
{
  printf(s);
  exit(0);
}

void xfer_init(void)
{
  struct termios serialsettings;
  struct termios serial_originalportsettings;
  struct sigaction cleanup;
  char linebuff[255];
  int nr, l;

  unsigned char ch;
  char version[255];
  int inver;
  
  memset ((void *) &serialsettings, 0, sizeof (struct termios));
  
  /*cleanup.sa_handler = (void (*)()) exit;
    sigemptyset (&cleanup.sa_mask);
    cleanup.sa_flags = 0;   
    
    sigaction (SIGINT, &cleanup, NULL); 
    sigaction (SIGHUP, &cleanup, NULL);
    sigaction (SIGTERM, &cleanup, NULL);
    sigaction (SIGSEGV, &cleanup, NULL);
    sigaction (SIGBUS, &cleanup, NULL);
    */

  ser_fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
  
  printf("[Opening serial..");
  if ( (ser_fd = open(device, O_SYNC | O_RDWR)) == -1 )
    {
      printf("Can't open %s!\n", device);
      exit(0);
    }

  /*fcntl (ser_fd, F_SETFL, 0);*/
  if (tcgetattr (ser_fd, &serial_originalportsettings) == -1)
    serial_error ("failed to get serial port attributes");

  if (cfsetispeed (&serialsettings, B115200))   /* used to be B9600 */
    serial_error ("failed to set input baud rate");
  if (cfsetospeed (&serialsettings, B115200))
    serial_error ("failed to set output baud rate");
  
  serialsettings.c_oflag &= ~OPOST;
  serialsettings.c_iflag = IGNPAR | IGNBRK;
  serialsettings.c_cflag = (serialsettings.c_cflag & ~CSIZE) | CS8 | CREAD | CLOCAL;
  serialsettings.c_cflag &= ~CRTSCTS;

  serialsettings.c_cc[VMIN] = 1;
  serialsettings.c_cc[VTIME] = 5;   /* <-wait time*/
  
  if (tcsetattr (ser_fd, TCSANOW, &serialsettings))
    serial_error ("failed to update serial port attributes");

  
  printf(".done, fd = %d.]\n", ser_fd);
  
  //Wait for arduino clones to reset
  sleep(opendelay);
  
  /* Check for cable.  Sending 255,255,255 will ensure it's in
     command mode.  '1' will return version string, and '2' puts
     it into multilink mode.
     */

  ch = 255;
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write probe byte 1!\n");
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write probe byte 2!\n");
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write probe byte 3!\n");

  /* Should be in command mode now: ask for version */
  ch = 1;
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write version check byte!\n");
  
  printf("[Checking cable version: \'");
  fflush(stdout);

  inver = 0;
  while (1)
    {
      if (read(ser_fd, &ch, 1) != 1)
	serial_error("Can't read version byte. :(\n");
  
      if (ch == '[')
	{
	  inver = 1;
	  continue;
	}

      if ( (inver) && (ch == ']') )
	break; /* read whole version string! */

      printf("%c", ch);
    }
  
  printf("\']\n");

  /* Ok.. maybe we've found the cable...... :) */

  /* Put it into multiboot mode: */

  ch = 2;
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write MB mode byte!\n");
  
  /* That's it... */
}

void xfer_passthru(void)
{
  char ch;

  printf("-> Setting cable to passthru at 115k2..");
  ch = 255;
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write escape for passthru!\n");

  /* Should be in command mode now: ask for passthru */
  ch = 3;
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write passthru command!\n");
  
  printf(".done!\n\n");
}

void xfer_file(char *fname)
{
  FILE *fp;
  unsigned char ch;
  int len, tlen;
  char code[256*1024];
  int codelen;

  char inbuff[256];
  char sumbuff[16];
  int sum;
  int offset;
  int i;


  fp = fopen(fname,"rb");
  if (!fp) 
    {
      printf("error - no such file: %s\n",fname); 
      exit(0);
    }
  
  codelen = fread(code, 1, 256*1024, fp);      //load up to 256k to buffer
  printf("-> Loaded %d bytes from %s.\n", codelen, fname);
  fclose(fp);
  
  printf("Talking to monitor\n");
  ch = '\n';
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write LF for monitor test!\n");

  /* OK, aDDDDDDDD = set adddress, s<XFERBLOCK bytes> = send block etc.
     and j = execute stuff */

  /* address is 0 by default */

  

  for (offset = 0; offset <= codelen; offset += XFERBLOCK)
    {
      len = read(ser_fd, &ch, 1);
      if (len != 1)
	serial_error("couldn't read monitor prompt\n");

      if (ch != '>')
	{
	  printf("Monitor prompt was %c instead of >!\n", ch);
	  exit(0);
	}

      sum = 0;
      
      ch = 's';
      if (write(ser_fd, &ch, 1) != 1)
	serial_error("Couldn't 's' for block!\n");

      for (i = 0; i < XFERBLOCK; i++)
	{
	  ch = code[offset + i];

	  sum += ch;

	  if (write(ser_fd, &ch, 1) != 1)
	    serial_error("Couldn't write data byte!\n");
	}

      /* Wait for reply: %08x saying sum of block: */
      len = 0;
      do
	{
	  tlen = read(ser_fd, &(inbuff[len]), 1);
	  len += tlen;
	} while (len < 8);

      if (len != 8)
	{
	  printf("couldn't read block checksum (len = %d)\n", len);
	  exit(1);
	}
      
      inbuff[8] = 0;

      /* Check that buff == my sum */
      sprintf(sumbuff, "%08x", sum);

      if (strcmp(sumbuff, inbuff) != 0)   // Have a > to start with
	{
	  printf("My checksum (%s) differs from GBA's (%s) for block %x!\n", sumbuff, inbuff, offset);
	  exit(1);
	}
      else
	{
	  /* Else sent block ok! */
	  //printf("offset %x checksum %08x\n", offset, sum);
	  printf("Offset %08x sum %08x [", offset, sum);

	  for (i = 0; i < 40; i++)
	    {
	      if ( (float) i < (((float)offset / (float)codelen) * 40.0) )
		printf("#");
	      else
		printf(" ");
	    }
	  printf("]\r");
	}
	  
    }

  printf("\n\nRunning code..");
  ch = 'j';
  if (write(ser_fd, &ch, 1) != 1)
    serial_error("Couldn't write 'jump' command!\n");

  
  printf(".done!\n\n");


}





unsigned int xfer(unsigned int in)
{
  unsigned int  out;
  int len, want;

  unsigned char low, hi, tmp;

  low = in & 0xff;
  hi = (in >> 8) & 0xff;

  if (low == 255)
    {
      len = write(ser_fd, &low, 1);
      tmp = 0;
      len += write(ser_fd, &tmp, 1);
      want = 2;
    }
  else
    {
      len = write(ser_fd, &low, 1);
      want = 1;
    }

#ifdef DEBUG
  printf("Wrote [%02x] ", low);
#endif

  if (hi == 255)
    {
      len += write(ser_fd, &hi, 1);
      tmp = 0;
      len += write(ser_fd, &tmp, 1);
      want += 2;
    }
  else
    {
      len += write(ser_fd, &hi, 1);
      want += 1;
    }



#ifdef DEBUG
   printf("[%02x] (%d)\n", hi, len);  
#endif

  if ( len != want )
    {
      printf("Couldn't write all of %x to ser., (len = %d, errno = %d)\n", in, len, errno);
      exit(0);
    }

  /* then get two bytes back: */
 
  len = read(ser_fd, &low, 1);

  len += read(ser_fd, &hi, 1);

#ifdef DEBUG
    printf ("read %d bytes, [%02x][%02x]\n", len, low, hi);
#endif 

  out = low | (hi << 8);
  return out;
}
