#include "LPC23xx.h"

/**********************************************************************************************************
			Routine to set processor and pheripheral clock 
***********************************************************************************************************/

void  TargetResetInit(void)
{
  // 72 Mhz Frequency. Similar to ADC
  if ((PLLSTAT & 0x02000000) > 0)
  {
      /* If the PLL is already running */
      PLLCON  &= ~0x02; 		/* Disconnect the PLL                     			*/
      PLLFEED  =  0xAA;                 /* PLL register update sequence, 0xAA, 0x55   			*/
      PLLFEED  =  0x55;
  }
  PLLCON   &= ~0x01;         		/* Disable the PLL                   				*/
  PLLFEED   =  0xAA;         		/* PLL register update sequence, 0xAA, 0x55  			*/
  PLLFEED   =  0x55;
  SCS      &= ~0x10;         		/* OSC RANGE = 0, Main OSC is between 1 and 20 Mhz  		*/
  SCS      |=  0x20;              	/* OS CEN = 1, Enable the main oscillator      			*/
  while ((SCS &  0x40) == 0);
  CLKSRCSEL = 0x01;                   	/* Select main OSC, 12MHz, as the PLL clock source      	*/
  PLLCFG    = (24 << 0) | (1 << 16); 	/* Configure the PLL multiplier and divider                 	*/   
  PLLFEED   = 0xAA;                 	/* PLL register update sequence, 0xAA, 0x55        		*/
  PLLFEED   = 0x55;
  PLLCON   |= 0x01;                     /* Enable the PLL                                           	*/
  PLLFEED   = 0xAA;                    	/* PLL register update sequence, 0xAA, 0x55                 	*/
  PLLFEED   = 0x55;
  CCLKCFG   = 3;                        /* Configure the ARM Core Processor clock divider           	*/
  USBCLKCFG = 5;                    	/* Configure the USB clock divider                          	*/
  while ((PLLSTAT & 0x04000000) == 0);  
  PCLKSEL0  = 0xAAAAAAAA;               /* Set peripheral clocks to be half of main clock    		*/
  PCLKSEL1  = 0x22AAA8AA;
  PLLCON   |= 0x02;                    	/* Connect the PLL. The PLL is now the active clock source  	*/
  PLLFEED   = 0xAA;                    	/* PLL register update sequence, 0xAA, 0x55                */
  PLLFEED   = 0x55;
  while ((PLLSTAT & 0x02000000) == 0);  
  PCLKSEL0 = 0x55555555;  		/* PCLK is the same as CCLK */
  PCLKSEL1 = 0x55555555;  
}

// serial Reception routine
int serial_rx(void) 
{ // pp421 LSR is RO 8-bit register in which b0 is set only if Receive Buffer Register (RBR) contains valid data
	while (!(U0LSR & 0x01)); /* UARTn Line status register. If (RDR!=1) wait for Rx to be over */
	return (U0RBR); /* Once RBR contains valid received data, read it */ 
}

//serial transmission routine 
void serial_tx(int ch)
{ // pp421 LSR is RO 8-bit register in which b5 Transmit Holding Register Empty (THRE) is set to 1 if THR is empty
//  while ((U0LSR & 0x20)!=0x20); THR - transmit holding register. == checks a bit or 0x**?
  while ((U0LSR & 0x20)==0); /* UARTn Line status register. If (THRE!=1) wait for tx to be over */
  U0THR = ch; // Once THR is empty, send the next ch to transmit
}

// serial transmission routine for string of characters
void string_tx(char *a)
{
      while(*a!='\0') // untill end, keep transmitting
      {
        while((U0LSR&0X20)!=0X20); /* As long as (THRE!=1), wait (for tx to be over) */
        U0THR=*a;  // once previous ch tx is over, take the next ch
        a++; // increment the address of ptr a to get the next ch
      }
}
/************************* main routine ************************************************************/
int  main ()
{
	unsigned int Fdiv;
	char value;
	TargetResetInit(); // initialization

	/**************************** uart1 initialization ************************************/	
	PINSEL0 = 0x00000050; // Pin selection for uart tx (5:4) & rx (7:6) lines. Table 106 pp157. 
	                      // for pin names, refer pp11 - block diagram
	U0LCR = 0x83; // pp419 WO 8 bit register, b7 DLAB=1, no Parity, 1 Stop bit  
	Fdiv = ( 72000000 / 16 ) / 19200 ; // Given baud rate being 19200, dividers DL_est computation   
	//Fdiv = ( 72000000 / 16 ) / 2400 ; // If DL_est is an integer (see flowchart pp427), then 
	                                    // quotient is U0DLM & remainder is U0DLL. For fraction?????  
	U0DLM = Fdiv / 256; // pp414 Division Latch MSB register - quotient
	U0DLL = Fdiv % 256; // pp428 Exmpls Division Latch LSB register - remainder
  	U0LCR = 0x03;               // pp419 DLAB = 0 to disable access to divisor latches
 
  	while(1) 
	{
		value=serial_rx(); // task given is to Rx & add 2 to it & Tx 
		serial_tx(value+2); 
	}
        return 0;
}



