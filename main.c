#include <msp430g2553.h>
#define     BUTTON                BIT3

/*

Main code to load onto the MSP430

The idea of running 2 timers simultaneously and storing the edges in an accumulator variable from 
Antscran's post on http://forum.43oh.com/topic/3317-msp430f550x-based-frequency-meter

Functionality:
Run TA0 and TA1 simultaneously, with TA0 counting to 10 000 for 25 times and TA1 on capture
mode so that we count how many times we encounter a falling edge.

The variable Peaks stores the number of crests in the signal for every frequency measurement;
since the UART can only transfer 8 bits at a time, create Peaks2 so that when Peaks reaches 250,
zero Peaks and add 1 to Peaks2. In the end, total number of peaks captured during a measurement 
equals Peaks2*250 + Peaks. 

The program is set up to make a measurement of frequency every 0.25 sec:
Frequency = number of cycles / sec = (total peaks) * 4 / 0.25sec.

Also, the button is set up to transfer 2 measurements of frequency = 0 to computer when pressed, 
which will be interpreted as an eighth note rest.

*/


volatile unsigned int Counter = 0;
volatile unsigned int Peaks = 0;
volatile unsigned int Peaks2 = 0;
unsigned int TXByte;

void InitializeButton(void);


void main(void) {

	/* Set up clock */
	WDTCTL = WDTPW + WDTHOLD;			// Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;                          // Calibrate DCO to 1MHz
	DCOCTL = CALDCO_1MHZ;				
	BCSCTL2 &= ~(DIVS_3);                           // SMCLK = DCO = 1MHz  
  
	InitializeButton();					
 
	/* Configure hardware UART */
	P1SEL = BIT1 + BIT2 ; 
	P1SEL2 = BIT1 + BIT2 ; 
	UCA0CTL1 |= UCSSEL_2; // Use SMCLK
	UCA0BR0 = 104; // Set baud rate to 9600 with 1MHz clock
	UCA0BR1 = 0; // Set baud rate to 9600 with 1MHz clock
	UCA0MCTL = UCBRS0; // Modulation UCBRSx = 1
	UCA0CTL1 &= ~UCSWRST; // Initialize USCI state machine

	/* ADC set up */
	P1DIR &= ~BIT7; 			//set P1.7 as input 
	P1DIR |= BIT0; 				// set up red LED on LaunchPad
	ADC10CTL0 = ADC10SHT_2 + ADC10ON;	// set register value:
						//  ADC10SHT_2 sets sample and 
						//  hold time at 16xADC10CLKs
						//  and ADC10ON turns ADC10
						//  converter on
										
	ADC10CTL1 = INCH_7; 			// set ADC's input channel to A7 (P1.7)
	ADC10AE0 |= BIT7; 			// Enable P1.7 for analog input
	
	
	P2DIR &= ~BIT0;				// set P2.0 as an input
	P2SEL |= BIT0;				// P2.0 set to Timer1_A

	/*** Timer_A Set-Up ***/
	TA0CCR0 = 10000;			// 10000*25=250000=0.25MHz; do 4 measurements per sec
	TA0CCTL0 |= CCIE;			// Interrupt enable
	TA0CTL |= TASSEL_2;			// select SMCLK as clock source

	TA1CCTL0 |= CCIE + CCIS_0 + CM_2 + CAP;	// Interrupt enable, capture select CCI0A (P2.0), 
						// capture on falling edge
	TA1CTL |= TASSEL_2;			// select SMCLK as clock source

	_BIS_SR(GIE);				// Enter low power mode with interrupts enabled

	/* Main program */
	while(1)
	{
		ADC10CTL0 |= ENC + ADC10SC;		// Sampling and conversion start

		while (ADC10CTL1 & ADC10BUSY);		// while ADC10BUSY and there's input

		if (ADC10MEM >= 0xCB) { //if input high enough
			__delay_cycles(10);			// Wait a bit (to account for the fact that when we sing a note, the first bit is almost 
									//always slightly off-pitch and we adjust to the correct pitch)
			TA0CTL |= MC0;				// Start timer		
			TA1CTL |= MC1;				// Start timer
			while(Counter != 25);			// Wait for 0.25 sec, i.e. do a measurement every ~0.25 sec
			TA1CTL &= ~MC1;				// Stop timer
			TA0CTL &= ~MC0;				// Stop timer

			if ((Peaks > 0) || (Peaks2 > 0)) { //If Timer_A1 captured anything, transfer the data
								// This is to prevent transferring frequency=0 (i.e. a rest) without pressing the button
		
				P1OUT |= 0x01;			// Turn on red LED on LaunchPad
				
				// Transfer the frequencies
				TXByte = (unsigned char)( Peaks );
				while (! (IFG2 & UCA0TXIFG));   // wait for TX buffer to be ready for new data
				UCA0TXBUF = TXByte;
			
				TXByte = (unsigned char)( Peaks2 );
				while (! (IFG2 & UCA0TXIFG));   // wait for TX buffer to be ready for new data
				UCA0TXBUF = TXByte;
				
				P1OUT &= ~0x01;			// Turn off LED
			}
			
			// Zero everything
			Counter = 0;			
			Peaks = 0;				
			Peaks2 = 0;
			TA0R = 0;					
			TA1R = 0;					
		}
	}
}
                           
//Timer_A0 Interrupt Service Routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer0_A0(void)
{
	Counter++;
}

//Timer_A1 Interrupt Service Routine
#pragma vector=TIMER1_A0_VECTOR
__interrupt void Timer1_A0(void)
{
	if (Peaks == 250) {
		Peaks2++;
		Peaks = 0;
	}
	else {
		Peaks++;
		} 	
}



void InitializeButton(void)   // Configure Push Button
{
  P1DIR &= ~BUTTON;
  P1OUT |= BUTTON;
  P1REN |= BUTTON;
  P1IES |= BUTTON;
  P1IFG &= ~BUTTON;
  P1IE |= BUTTON;
}



/* *************************************************************
 * Port Interrupt for Button Press 
   Adds an eighth note rest 
   (i.e. add two measurements of frequency = 0)
 * *********************************************************** */
#if defined(__TI_COMPILER_VERSION__)
#pragma vector=PORT1_VECTOR
__interrupt void port1_isr(void) 
#else
  void __attribute__ ((interrupt(PORT1_VECTOR))) port1_isr (void)
#endif
{  
	P1IFG &= ~BUTTON; 		 // clear out interrupt flag
	P1OUT |= 0x01;			 // Turn on red LED to indicate data is going to be transferred to laptop
	
	TXByte = (unsigned char)( 0 );
	while (! (IFG2 & UCA0TXIFG));    // wait for TX buffer to be ready for new data
	UCA0TXBUF = TXByte;

	TXByte = (unsigned char)( 0 );
	while (! (IFG2 & UCA0TXIFG));   // wait for TX buffer to be ready for new data
	UCA0TXBUF = TXByte;	
	P1OUT &= ~0x01;                // Turn off LED
}