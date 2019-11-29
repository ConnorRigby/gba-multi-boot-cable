//-----------------------------------------------------------------------//
// MULTI.C                                                               //
//    -What is it?                                                       //
//        This is some c-code for sending a program to the Gameboy       //
//        Advance using a multiboot cable.                               //
//        The actual transfer is not included in this file because there //
//        is more then one way to implement the transfering.  The most   //
//        reliable way is to have a smart cable that will be controled   //
//        by the parallel port and convert the data to serial.  But the  //
//        cheapest way is to use DOS and have the PC do the timing.      //
//        The communications is 16-bit serial with one start bit and     //
//            one stop bit. (See XFER.C)                                 //
//    -disassembled by someone (posted on www.godsmaze.org/gba/)         //
//        probably used IDA to disassemble                               //
//    -de-compiled (by hand... phew) and commented by:                   //
//        Andrew May                                                     //
//            EMAIL:    android_1978@hotmail.com                         //
//            WEB:    http://program.at/Andrew                           //
//        Miguel Angel Ajo						 //
//	      EMAIL:    ajo@godsmaze.org			         //
//            WEB:    http://www.godsmaze.org/gba			 //
//    -Reason for decompilation:                                         //
//        I object to people making money off something that is illegal  //
//        that includes programming for the GBA.  Any information that   //
//        people have in the underground programming of such devices     //
//        should be shared within the community. Hence what I am doing.  //
//    -you can use this code for whatever purposes (good or 'EVIL') but  //
//	 I take no responsability for any injury, damage to property or  //
//	 copywrite breaches.                                             //
//    The Decompilation of this software was done simply as an exercise  //
//    with no commercial intent.  Those wishing to use this to make a    //
//    commercial product should probably get in touch with nintendo and  //
//    and Jeff Frohwein as I believe that it was his original code that  //
//    was disassembled.  Visit - HTTP://www.devrs.com/gba                //
//-----------------------------------------------------------------------//

//#include "mbheader.h"
#include <stdio.h>

//unsigned char Client[256*1024];
//int ClientLength;
extern unsigned char Client[];
extern int ClientLength;

// External Data references

extern int verbose;
extern int verbose2;

// External Process references
#define TimerDelay(n) { int i; for (i = 0; i < 10000000*n; i++) {;} }

//extern void TimerDelay(unsigned int);
extern void ProgramExit(unsigned int);

int multi(int Input)
{
    unsigned int bit;
    unsigned int data32;
    unsigned int var_30;
    unsigned int client_pos;
    unsigned int still_sending;
    unsigned int send_data16;
    unsigned int encrypt_seed;
    unsigned int send_data16_high = 0;
    unsigned int columns = 0;
    unsigned int var_C;
    unsigned char var_8;
    unsigned char var_1;

    unsigned int eax, ebx;
    unsigned int RData, Counter, CRCTemp;
    unsigned gbatimeout=10;

    ClientLength = (ClientLength + 0x0F) & 0xFFFFFFF0; // Align ClientLength to 16
    if ( ClientLength < 0x1C0 ) ClientLength = 0x1C0;  // Minimum length is 0x1C0
    
    RData = 0;
    while ((RData != 0x7202) && gbatimeout)	  // Get GBA attention
    {				   		  //  
        gbatimeout--;
        TimerDelay(1);			          //   
	RData = xfer(0x6202);			  //   
	if (verbose) printf("%x ", RData);	          //  
    }						  // 

    if (RData !=0x7202) {
    		printf("Timeout waiting for GBA\n");
    		return 1;
    }
    
    if (verbose2) printf("\n");
    if (verbose) printf("Sending data. Please wait...\n");

    RData = xfer(0x6100); 				  // command: about to send header
    if (verbose2) printf("%x\n",RData);
    for (Counter = 0; Counter<=0x5F; Counter++) 	  // Send 0x5F of data with no encryption
    {
        RData = xfer((Client[(Counter*2)+1]<<8)+Client[Counter*2]);	// two bytes are concatenated.
        if (verbose2) printf("%x", RData);
    }
	
    if (verbose2) printf("\n");

    RData = xfer(0x6202);				// about to send command.
    
    if (verbose2) printf("client_data=%x\n",RData);
    RData = xfer(0x63C1);				// Command: send encryption value
    
    if (verbose2) printf("dlr=%x\n",RData);
    RData = xfer(0x63C1);				// Get encryption value
    
    if (verbose2) printf("dlr=%x\n",RData);

	
    encrypt_seed = ((RData&0x0FF)<<8)|0x0FFFF00C1;     // Looks like an encryption value
    var_1 = (RData&0x0FF)+0x20F;
    RData = xfer((var_1 & 0x000000FF)|0x00006400);    // Encryption confirmation???
    if (verbose2) printf("sdl=%x\n",RData);


//    TimerDelay(1);
//    RData = xfer(((ClientLength+0x0FFFFFF40)>>2)+0x0FFFFFFCC); 
	
	
    
    TimerDelay(1);
    RData = xfer( ( (ClientLength-0xC0) >> 2 ) - 0x34 );  // Send ClientLength (some encryption thing)
	                                                
    if (verbose2) printf("str5=%x\n",RData);

    var_8 = RData;		// Setup variables for encrypted data transfer
    var_C = 0x0FFF8;            // 	
    
    client_pos = 0x0C0;		//  send after header  	
    
    still_sending = 0x2;        //  2 end signals

//---------------------------- Send encrypted data -------------------------------
//--------------------------------------------------------------------------------
    do
    {
        send_data16 = send_data16_high; // Prepare to xfer bits31;16 (only useful every second time)
        if (!(client_pos&0x02))			// every 'odd' time through (iteration 1,3,5,...)
        {
            data32 = Client[client_pos] + (Client[client_pos+1] << 8)             // Get 4 bytes(32-bits)
                   + (Client[client_pos+2]<<0x10) + (Client[client_pos+3] <<0x18);// - to encrypt/send
                   
            CRCTemp = data32;
            
            for (bit = 0; bit <= 31; bit++)               // CRC Calculation (???)
            {						  // 
                var_30 = var_C^CRCTemp;                   // 
		var_C = var_C >> 1;			  // 
		CRCTemp = CRCTemp >> 1;                   // 
		if (var_30&0x01) var_C = var_C^0x0A517;	  // 
	    }						  // 
	    
            encrypt_seed = (encrypt_seed * 0x6F646573)+1; // Calculate new encryption value
            
            send_data16 = (encrypt_seed ^ data32)                  // encrypt data
                     ^( (~(client_pos+0x2000000)+1) ^ 0x6465646F); //
                     
	    send_data16_high = send_data16 >> 16; // The upper 16 bits to be sent 
	                                          // in second transfer
	                                          
            send_data16 = send_data16&0x0FFFF;    
         }
//--------------------------------------------------------------------------------
         while(1)
         {
             if (client_pos != 0x0C0)				// if already sent (client_pos!=0xc0)
	     {					  	        //  
                 if (RData != ((client_pos-2)&0x0FFFF))         //  Check the Return value
                 {						//  (address of last data sent)
                    printf("Transmision error\n");		//   should be client_pos-2
                    return 1; 	                                //  
		 }
	     }
	     RData = xfer(send_data16);	 		        // Send 16-bits of encrypted data
	     if (verbose2) printf("%x:%x ",RData,send_data16);  //
             else
             
             if (!(client_pos%127))  {
             		printf("%g k.     \b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",((float)client_pos)/(1024.));
             		fflush(stdout);
             	}
	 
	     columns++;		                                // Print 8 transfers per line
	     if ((verbose2)&&(columns==7))				//  
	     {		 				        //  
	        columns = 0;					//  
  	        printf("\n");					//
	     }							// 
	 
	     if (still_sending)
	     {
	        client_pos = client_pos + 2;			// Next 16-bits of data
                if ((still_sending==2)&&(client_pos!=ClientLength)) break;	// Still doing data transfer
		send_data16 = 0x65;			// The end of data signal to be sent twice
		still_sending --;			// Controls  Data/Enddata/Enddata
	     }
	     else break;
        }
    } while(still_sending);

//------------------------------- CRC checking -----------------------------------
//--------------------------------------------------------------------------------

	while (RData != 0x0075) RData = xfer(send_data16);	// While no ack from the GBA

	send_data16 = 0x0066;		  		        // Send transfer end signal
	                                        
	RData = xfer(send_data16);

        data32 = ((((RData&0xFF00)+var_8)<<8)|0xFFFF0000)+var_1;    //  CRC value
	for (bit = 0 ; bit<=31 ; bit++)		  	            //
	{							    //
		var_30 = var_C ^ data32;		 	    //
		var_C  = var_C>>1;				    //
		data32 = data32>>1;	 			    // 
		if (var_30 & 0x01) var_C = var_C ^ 0x0A517;  	    //
	}
	
	RData = xfer(var_C); 	  				    // Send CRC value
        
	if (verbose2) printf("[[[%x:%x]]]\n", RData,var_C);         
	
	if (var_C != RData)
	{
		printf("Transmision error: CRC Bad!.\n");
		return 1;
	}
	if (verbose) printf("CRC Ok - Transmision Done.\n");
	return 0;
}



