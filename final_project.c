// =================================================================
// Sean Mitchell
// CPE 325-5 SP2019
// Final Project - Digital Music Synthesizer
// Inputs: None
// Outputs: Analog Audio
// UART connection: 19200
// =================================================================
#include <msp430.h>
#include <stdio.h>
#include <string.h>

#include <sine_lut_256.h>       /*256 samples are stored in this table */
#include <tri_lut_256.h>        /*256 samples are stored in this table */
#include <saw_lut_256.h>        /*256 samples are stored in this table */
#include <square_lut_256.h>     /*256 samples are stored in this table */

// Text messages
char greetMessage1[] = "\nCPE 325 Final Project: Digital Music Synthesizer";
char greetMessage2[] = "Instructions: Enter a musical note (C, D, E, F, G, A, B) and press enter";
char greetMessage3[] = "Press ? to see what waveform is currently being generated";
char greetMessage4[] = "Press switch 1 on the MSP430 to change what waveform is being generated";
char errorMessage[] = "Error: you must enter a valid musical note";

unsigned int i = 0;
int waveform_flag = 0;                  // Counter for which waveform is currently selected
char tmp[32];                           // Actual recieved string
unsigned int global_i;                  // Counter for input
unsigned char ch[32];                   // Hold char from UART RX

// Input comparisons
char case_1[] = {"A"};
char case_2[] = {"B"};
char case_3[] = {"C"};
char case_4[] = {"D"};
char case_5[] = {"E"};
char case_6[] = {"F"};
char case_7[] = {"G"};
char case_status[] = {"?"};

// Outputs
char waveform_1[] = "sinewave";
char waveform_2[] = "sawtooth wave";
char waveform_3[] = "triangle wave";
char waveform_4[] = "square wave";

// Timer values for each note
int C_NOTE_FREQ = 256;
int D_NOTE_FREQ = 227;
int E_NOTE_FREQ = 195;
int F_NOTE_FREQ = 186;
int G_NOTE_FREQ = 163;
int A_NOTE_FREQ = 146;
int B_NOTE_FREQ = 132;

void TimerA_setup(void) {
    TACTL = TASSEL_2 + MC_1;              // SMCLK, up mode
    TACCR0 = C_NOTE_FREQ;                 // Sets Timer Freq (1048576*(NOTE_FREQ)/256)
    TACCTL0 = CCIE;                       // CCR0 interrupt enabled
}

void DAC_setup(void) {
    ADC12CTL0 = REF2_5V + REFON;          // Turn on 2.5V internal ref volage
    unsigned int i = 0;
    for (i = 50000; i > 0; i--);          // Delay to allow Ref to settle
    DAC12_0CTL = DAC12IR + DAC12AMP_5 + DAC12ENC + DAC12OPS;   //Sets DAC12
    DAC12_0DAT = 0x057F;                      // Offset level
}

void OA_setup()
{
    // Setup OA1 = S-K Output filter for DAC12_1
    // OA1+ = P6.4/OA1I0
    // OA1- = R_Bottom (internal)
    // OA1OUT = P6.3/A3/OA1O, R_Bottom (internal)
    OA1CTL0 = OAPM_3 + OAADC1;                // Select inputs, power mode
    OA1CTL1 = OAFC_1 + OARRIP;                // Unity Gain, rail-to-rail inputs

    // Setup OA2 = Gain stage
    // OA2+ = DAC12_0OUT
    // OA2- = OA1OUT
    // OA2OUT = P6.5/OA2O
    OA2CTL0 = OAN_2 + OAP_2 + OAPM_3 + OAADC1;// Select inputs, power mode
    OA2CTL1 = OAFC_6 + OAFBR_2 + OARRIP;      // Invert. PGA, OAFBRx sets gain
}

void UART_initialize(void)
{
    P2SEL |= BIT4 + BIT5;          // Set UC0TXD and UC0RXD to transmit and receive data
    UCA0CTL1 |= UCSWRST;           // Software reset
    UCA0CTL0 = 0;                  // USCI_A0 control register
    UCA0CTL1 |= UCSSEL_2;          // Clock source SMCLK - 1048576 Hz
    UCA0BR0 = 54;                  // Baud rate - 1048576 Hz / 19200
    UCA0BR1 = 0;
    UCA0MCTL = 0x0A;               // Modulation
    UCA0CTL1 &= ~UCSWRST;          // Software reset
    IE2 |= UCA0RXIE;               // Enable USCI_A0 RX interrupt
}

void UART_putCharacter(char c)
{
    while (!(IFG2 & UCA0TXIFG));    // Wait for previous character to transmit
    UCA0TXBUF = c;                  // Put character into tx buffer
}

char UART_getCharacter()
{
    char c;
    c = UCA0RXBUF;
    return c;
}

void UART_sendMessage(char* message) {
    int i;
    for(i = 0; message[i] != 0; i++) {
        UART_putCharacter(message[i]);
    }
    UART_putCharacter('\n');       // Newline
    UART_putCharacter('\r');       // Carriage return
}

void UART_receiveLine(char* buffer, int limit)
{
    for (int j = 0; j < global_i-1; j++)
    {
        tmp[j] = ch[j];
    }
}

int main(void)
{
    WDTCTL = WDTPW+WDTHOLD;             // Stop WDT
    TimerA_setup();                     // Setup TimerA
    DAC_setup();                        // Configure DAC12
    OA_setup();                         // Configure Op-Amps for analog audio output
    UART_initialize();                  // Configure UART
    _EINT();                            // Enable interrupts
      
  	P1IE |= BIT0+BIT1;                  // interrupt enable
	P1IES |= BIT0+BIT1;                 // interrupt edge select
	P1IFG &= ~BIT0+BIT1;                // interrupt flag

    for(int z = 100; z > 0; z--);       // Delay to allow baud rate stabilize
    
    // Send text messages
    UART_sendMessage(greetMessage1);
    UART_sendMessage(greetMessage2);
    UART_sendMessage(greetMessage3);
    UART_sendMessage(greetMessage4);

    // Main loop
    while (1)
    {
        //__bis_SR_register(LPM0_bits + GIE); // Enter LPM0, interrupts enabled
        if (waveform_flag == 0)
        {
            //printf("flag_0\n");
            DAC12_0DAT = SINE_LUT256[i];
        }
        else if (waveform_flag == 1)
        {
            //printf("flag_1\n");
            DAC12_0DAT = TRI_LUT256[i];
        }
        else if (waveform_flag == 2)
        {
            //printf("flag_2\n");
            DAC12_0DAT = SAW_LUT256[i];
        }
        else if (waveform_flag == 3)
        {
            //printf("flag_2\n");
            DAC12_0DAT = SQUARE_LUT256[i];
        }
        i=(i+1)%256;
    }
}

#pragma vector = TIMERA0_VECTOR
__interrupt void TA0_ISR(void) 
{
    __bic_SR_register_on_exit(LPM0_bits);  // Exit LPMx, interrupts enabled
}

// Switch ISR
#pragma vector = PORT1_VECTOR
__interrupt void Port1_ISR(void)
{
    if (P1IFG == BIT0)         // Switch 1
    {
        for(unsigned int i = 0; i < 2000; i++);
        if (P1IFG == BIT0)
        {
            printf("SW1, ");
            if (waveform_flag < 3)
                waveform_flag++;
            else
                waveform_flag = 0;
            //printf("Flag: %d\n", flag);
            P1IFG &= ~BIT0;
            P1IES |= BIT0;
        }

    }
    else if (P1IFG == BIT1)         // Switch 2
    {
        for(unsigned int i = 0; i < 2000; i++);
        if (P1IFG == BIT1)
        {
            printf("SW2\n");
            /*
            printf("SW2, ");
            if (flag < 3)
                flag++;
            else
                flag = 0;
            printf("Flag_2: %d\n", flag);
            */
            //OA0CTL0 = 0;
            P1IFG &= ~BIT1;
            P1IES |= BIT1;
        }
    }
}

// Interrupt for USCI Rx
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIB0RX_ISR (void)
{
    P5OUT ^= BIT1;                          // Toggle LED4
    ch[global_i] = UCA0RXBUF;               // Character received is moved to a variable
    while(!(IFG2&UCA0TXIFG));               // Wait until TXBUF is free
    UCA0TXBUF = ch[global_i];               // TXBUF <= RXBUF (echo)
    global_i++;                             // increment global counter


    if (UCA0RXBUF == '\r')
    {
        UART_receiveLine(ch, 32);

        if (strcmp(case_status,tmp) == 0)
        {
            char status_str[32];
            switch(waveform_flag)
            {
                // Sets a string with the infomation of which waveform is currently being generated
                case 0:
                    sprintf(status_str, "\nWaveform now playing: %s",waveform_1);
                    break;
                case 1:
                    sprintf(status_str, "\nWaveform now playing: %s",waveform_2);
                    break;
                case 2:
                    sprintf(status_str, "\nWaveform now playing: %s",waveform_3);
                    break;
                case 3:
                    sprintf(status_str, "\nWaveform now playing: %s",waveform_4);
                    break;
                default:
                    break;
            }
            UART_sendMessage(status_str);
        }
        else if (strcmp(case_3,tmp) == 0)                   // 'C' received?
        {
            TACCR0 = C_NOTE_FREQ;                           // Sets Timer Freq (1048576*(NOTE_FREQ)/256)
            char status_str[32];
            sprintf(status_str, "\nNote now playing: C");
            UART_sendMessage(status_str);
        }
        else if (strcmp(case_4,tmp) == 0)                   // 'D' received?
        {
            TACCR0 = D_NOTE_FREQ;                           // Sets Timer Freq (1048576*(NOTE_FREQ)/256)
            char status_str[32];
            sprintf(status_str, "\nNote now playing: D");
            UART_sendMessage(status_str);
        }
        else if (strcmp(case_5,tmp) == 0)                   // 'E' received?
        {
            TACCR0 = E_NOTE_FREQ;                           // Sets Timer Freq (1048576*(NOTE_FREQ)/256)
            char status_str[32];
            sprintf(status_str, "\nNote now playing: E");
            UART_sendMessage(status_str);
        }
        else if (strcmp(case_6,tmp) == 0)                   // 'F' received?
        {
            TACCR0 = F_NOTE_FREQ;                           // Sets Timer Freq (1048576*(NOTE_FREQ)/256)
            char status_str[32];
            sprintf(status_str, "\nNote now playing: F");
            UART_sendMessage(status_str);
        }
        else if (strcmp(case_7,tmp) == 0)                   // 'G' received?
        {
            TACCR0 = G_NOTE_FREQ;                           // Sets Timer Freq (1048576*(NOTE_FREQ)/256)
            char status_str[32];
            sprintf(status_str, "\nNote now playing: G");
            UART_sendMessage(status_str);
        }
        else if (strcmp(case_1,tmp) == 0)                   // 'A' received?
        {
            TACCR0 = A_NOTE_FREQ;                           // Sets Timer Freq (1048576*(NOTE_FREQ)/256)
            char status_str[32];
            sprintf(status_str, "\nNote now playing: A");
            UART_sendMessage(status_str);
        }
        else if (strcmp(case_2,tmp) == 0)                   // 'B' received?
        {
            TACCR0 = B_NOTE_FREQ;                           // Sets Timer Freq (1048576*(NOTE_FREQ)/256)
            char status_str[32];
            sprintf(status_str, "\nNote now playing: B");   
            UART_sendMessage(status_str);
        }
        else                                                // Error: no valid character recieved
        {
            UART_sendMessage(errorMessage);
        }

        global_i = 0;
        memset(tmp, 0, sizeof tmp);         // clears tmp
    }
}
