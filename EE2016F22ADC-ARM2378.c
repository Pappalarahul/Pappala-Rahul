#include "LPC23xx.h"

void  TargetResetInit(void)
{
  // 72 Mhz Frequency
  if ((PLLSTAT & 0x02000000) > 0)
  {
      /* If the PLL is already running   */
      PLLCON  &= ~0x02;  		/* Disconnect the PLL                 			*/
      PLLFEED  =  0xAA;  		/* PLL register update sequence, 0xAA, 0x55         		*/
      PLLFEED  =  0x55;
  }
  PLLCON   &= ~0x01;     		/* Disable the PLL                           			*/
  PLLFEED   =  0xAA;     		/* PLL register update sequence, 0xAA, 0x55                 	*/
  PLLFEED   =  0x55;
  SCS      &= ~0x10;     		/* OSCRANGE = 0, Main OSC is between 1 and 20 Mhz           	*/
  SCS      |=  0x20;     		/* OSCEN = 1, Enable the main oscillator                    	*/
  while ((SCS &  0x40) == 0);
  CLKSRCSEL = 0x01;                  	/* Select main OSC, 12MHz, as the PLL clock source 	*/
  PLLCFG    = (24 << 0) | (1 << 16); 	/* Configure the PLL multiplier and divider                 */   
  PLLFEED   = 0xAA;                  	/* PLL register update sequence, 0xAA, 0x55                 */
  PLLFEED   = 0x55;
  PLLCON   |= 0x01;                    /* Enable the PLL                                           */
  PLLFEED   = 0xAA;                    /* PLL register update sequence, 0xAA, 0x55                 */
  PLLFEED   = 0x55;
  CCLKCFG   = 3;                       /* Configure the ARM Core Processor clock divider           */
  USBCLKCFG = 5;                    	/* Configure the USB clock divider                          */
  while ((PLLSTAT & 0x04000000) == 0);  
  PCLKSEL0  = 0xAAAAAAAA;          	/* Set peripheral clocks to be half of main clock           */
  PCLKSEL1  = 0x22AAA8AA;
  PLLCON   |= 0x02;                  	/* Connect the PLL. The PLL is now the active clock source  */
  PLLFEED   = 0xAA;              	/* PLL register update sequence, 0xAA, 0x55                 */
  PLLFEED   = 0x55;
  while ((PLLSTAT & 0x02000000) == 0);  
  PCLKSEL0 = 0x55555555;  		/* PCLK is the same as CCLK */
  PCLKSEL1 = 0x55555555;  
}


/********************* serial Transmission routine***********************************/	
void serial_tx(int ch)
{ // pp421 LSR is RO 8-bit register in which b5 Transmit Holding Register Empty (THRE) is set to 1 if THR is empty  
  while ((U0LSR & 0x20)!=0x20);	/* UARTn Line status register. If (THRE!=1) wait for tx to be over */
  U0THR = ch;	/* Transmit holding register. Once THR is empty, then send the next ch	*/
}
/**************************** Routine for converting hex value to ascii value *****************/	
int htoa(int ch)
{       /* refer ASCII table */
	if(ch<=0x09)
		ch = ch + 0x30;	/* For numerals hex 0 to 9 ASCII adds 0x30	*/
	else
		ch = ch + 0x37; /* For hex numerals A to F, characters only? */
	return(ch);
}
/*********************************** main routine ****************************************************/
int  main ()
{
	unsigned int Fdiv,value,i,j;
//	char value;
	TargetResetInit(); 
//	init_timer( ((72000000/100) - 1) );
	/* power control for peripherals (PCONP) 32 bit WO register. pp 68 */
	PCONP |=0X00001000;	// b12 of PCONP PCAD=1 to ENABLE A/D converter module  
	PINSEL0 = 0x00000050; // Pin selection for uart tx (5:4) & rx (7:6) lines. Table 106 pp157. 
	                      // for pin names, refer pp11 - block diagram
	PINSEL1	= 0X01554000; // Pin selection for AD0.0 (15:14) [nibble 0100], AD0.1 (17:16), AD0.2 
			      // (19:18), [nibble 0101], AD0.3 (21:20), SDA0 (23:22) [nibble 0101],
			      // SCL0 (25:24) [nibble 0001] concatenated to 0x01554000 - 8 bytes 
	                      // 4 channels in AD0, SDA0 (I2C serial data port), SCL0 I2C Serial Clk
	                      // Why I2C ports are invoked? 
	                      
	                      
	/************** Uart initialization **********************************/
                      // Line control register. To access Divisor latches (DLM & DLL), DLAB = 1
	U0LCR = 0x83; // pp419 WO 8 bit rgstr: 7 (DLAB)--> 1, 6-->0, 5:4-->00 (odd parity), b3--> 0 (no parity), 
	              // b2-->0 (1 Stop bit), b1:b0 --> 11 (8 bit character lngth),  
	Fdiv = ( 72000000 / 16 ) / 19200; // Given baud rate being 19200, dividers DL_est computation   
	//Fdiv = ( 72000000 / 16 ) / 2400 ; // If DL_est is an integer (see flowchart pp427), then 
	                                    // quotient is U0DLM & remainder is U0DLL. For fraction?????   
	U0DLM = Fdiv / 256; // pp414 Division Latch MSB register - quotient
	U0DLL = Fdiv % 256; // pp428 Exmpls Division Latch LSB register - remainder 
   	U0LCR = 0x03; // pp419 DLAB = 0 to disable access to divisor latches -   
	
  	AD0CR = 0X01210F01; // pp 598. ADC settngs: AD0.1 other AD0.* disabled, 4clks/3 bits?, start now 
	while(1) 
	{
		while((AD0DR0 & 0X80000000)!=0X80000000){}; // Wait here until adc make conversion complete
	
		/************* To get converted value and display it on the serial port****************/
            	value = (AD0DR0>>6)& 0x3ff ;    //ADC value
			//serial_tx(value);
		serial_tx('\t');
		serial_tx(htoa((value&0x300)>>8));
		serial_tx(htoa((value&0xf0)>>4));
		serial_tx(htoa(value&0x0f));
		serial_tx(0x0d); 
		serial_tx(0x0a);
			
			for(i=0;i<=0xFF;i++) /* delay?? */ 
			{
	    			for(j=0;j<=0xFF;j++);
			}
	}
  return 0;
}



