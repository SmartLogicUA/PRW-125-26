/*****************************************************
This program was produced by the
CodeWizardAVR V1.25.3 Professional
Automatic Program Generator
� Copyright 1998-2007 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : Boot Loader
Version : v9.3.x
Date    : 24.03.2009
Author  : T.Drozdovsky                           
Company : Smart Logic                              
Comments: 


Chip type           : ATmega8
Program type        : Application
Clock frequency     : 14,745600 MHz
Memory model        : Small
External SRAM size  : 0
Data Stack size     : 256
*****************************************************/

#include <mega8.h>
#include <string.h>    
#include <delay.h>
#include <board.h>     
#include <g28147.h>                                

#define IVCE    0
#define IVSEL   1

flash char FHandContr[]="SYHND 00049";                    // Get Handle Controller
#define VERSIA_BOOT_LOADER  "\r\nBootLoader v9.3.x"     

register unsigned char  Bufferl  @0;
register unsigned char  Bufferh  @1;

register unsigned int  reg_temp @4;

flash unsigned char *mas;

union {                                                 // union for conversion type long,int in char
  unsigned char byte[4];
  unsigned int word[2];
  unsigned long dword;      
 } number;

union {                                                 // union for check sum (interrupt)
 unsigned char crcb[2];
 unsigned int crcw;      
      } crci;      

unsigned char kkkk[]="Not correct CRC check sum";

#pragma warn+ 

eeprom unsigned char *pEEPROM;

unsigned int counter_PC=0;
unsigned char buffer_PC[275];

bit StartFlagPC=0;        

unsigned char BootHere[]="SBLFL";   

/*****************************************************************************/
/*                 G O S T   2 8 1 4 7 - 8 9                                 */
/*****************************************************************************/

unsigned long S[2];
unsigned long KZU[8]={0x4FEF9107,0x3D03CBD2,0x880C6A88,0x9CFF698A,  // KEY
                      0xD07A98B4,0xB02D19E3,0xFD42B5FB,0xDAC70259};
                      
unsigned char K[128]={       // Gam_c
   0x0F, 0x04, 0x03, 0x06, 0x0D, 0x02, 0x09, 0x08, 0x00, 0x0C, 0x01, 0x0E, 0x07, 0x05, 0x0A, 0x0B,
   0xE0, 0x30, 0xC0, 0x20, 0xD0, 0x90, 0x10, 0xB0, 0x50, 0x40, 0x60, 0xA0, 0x00, 0xF0, 0x80, 0x70,
   0x04, 0x08, 0x0C, 0x01, 0x0E, 0x07, 0x00, 0x0F, 0x0B, 0x02, 0x09, 0x06, 0x05, 0x03, 0x0A, 0x0D,
   0x10, 0x90, 0x80, 0x50, 0xE0, 0xF0, 0xA0, 0xB0, 0x00, 0xC0, 0x70, 0x30, 0xD0, 0x60, 0x20, 0x40,
   0x09, 0x03, 0x04, 0x0C, 0x01, 0x00, 0x0B, 0x05, 0x06, 0x0F, 0x0D, 0x0E, 0x07, 0x02, 0x0A, 0x08,
   0xE0, 0x40, 0x90, 0xD0, 0xA0, 0x30, 0xF0, 0x10, 0xB0, 0x70, 0x80, 0x50, 0x60, 0xC0, 0x20, 0x00,
   0x02, 0x0A, 0x06, 0x0E, 0x05, 0x0D, 0x0C, 0x03, 0x07, 0x08, 0x0F, 0x00, 0x09, 0x0B, 0x01, 0x04,
   0xB0, 0x90, 0x80, 0x10, 0xA0, 0x60, 0xD0, 0x40, 0x00, 0x50, 0x20, 0xC0, 0xE0, 0xF0, 0x30, 0x70};

/*****************************************************************************/
/*                  F U N C T I O N  P R O T O T Y P E                       */
/*****************************************************************************/
void Receive_PC(void);
void ProgramFlashPage(unsigned char *);
void ProgramEEPROM(unsigned char *);
void InitSign(unsigned char *);

void SendAnswerR(char *);
void Print(flash char *data);
        
void crc2(unsigned char *,unsigned int *,unsigned int);
unsigned char check_flash_crc(void);
void WriteFlash(unsigned int P_address,unsigned char *pData);  
void ASCIIToHex(unsigned char *,unsigned int);
/*****************************************************************************/  

flash char tascii[]={"0123456789ABCDEF"};  

#pragma warn+ 


#define SPMEN  0
#define PGERS  1
#define PGWRT  2
#define BLBSET 3
#define RWWSRE 4
#define RWWSB  6
#define SPMIE  7

#define EEWE   1 
#define EEPE   1                          

#define RXB8 1
#define TXB8 0
#define UPE 2
#define OVR 3
#define FE 4
#define UDRE 5
#define RXC 7

#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<OVR)
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)

// USART Receiver buffer
#define RX_BUFFER_SIZE 64
char rx_buffer[RX_BUFFER_SIZE];

#if RX_BUFFER_SIZE<256
unsigned char rx_wr_index,rx_rd_index,rx_counter;
#else
unsigned int rx_wr_index,rx_rd_index,rx_counter;
#endif

// This flag is set on USART Receiver buffer overflow
bit rx_buffer_overflow;

// USART Receiver interrupt service routine
interrupt [USART_RXC] void usart_rx_isr(void)
{
char status,data;
status=UCSRA;
data=UDR;
if ((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN))==0)
   {
   rx_buffer[rx_wr_index]=data;
   if (++rx_wr_index == RX_BUFFER_SIZE) rx_wr_index=0;
   if (++rx_counter == RX_BUFFER_SIZE)
      {
      rx_counter=0;
      rx_buffer_overflow=1;
      };
   };
}

#ifndef _DEBUG_TERMINAL_IO_
// Get a character from the USART Receiver buffer
#define _ALTERNATE_GETCHAR_
#pragma used+
char getchar(void)
{
char data;
while (rx_counter==0);
data=rx_buffer[rx_rd_index];
if (++rx_rd_index == RX_BUFFER_SIZE) rx_rd_index=0;
#asm("cli")
--rx_counter;
#asm("sei")
return data;
}
#pragma used-
#endif

// USART Transmitter buffer
#define TX_BUFFER_SIZE 64
char tx_buffer[TX_BUFFER_SIZE];

#if TX_BUFFER_SIZE<256
unsigned char tx_wr_index,tx_rd_index,tx_counter;
#else
unsigned int tx_wr_index,tx_rd_index,tx_counter;
#endif

// USART Transmitter interrupt service routine
interrupt [USART_TXC] void usart_tx_isr(void)
{
if (tx_counter)
   {
   --tx_counter;
   UDR=tx_buffer[tx_rd_index];
   if (++tx_rd_index == TX_BUFFER_SIZE) tx_rd_index=0;
   };
}

#ifndef _DEBUG_TERMINAL_IO_
// Write a character to the USART Transmitter buffer
#define _ALTERNATE_PUTCHAR_
#pragma used+
void putchar(char c)
{
while (tx_counter == TX_BUFFER_SIZE);
#asm("cli")
if (tx_counter || ((UCSRA & DATA_REGISTER_EMPTY)==0))
   {
   tx_buffer[tx_wr_index]=c;
   if (++tx_wr_index == TX_BUFFER_SIZE) tx_wr_index=0;
   ++tx_counter;
   }
else
   UDR=c;
#asm("sei")
}
#pragma used-
#endif

// Standard Input/Output functions
#include <stdio.h>

// Timer 1 output compare A interrupt service routine
interrupt [TIM1_COMPA] void timer1_compa_isr(void)
{
// Place your code here
    GREEN_TOG
    RED_TOG
}

// Declare your global variables here

void main(void)
{
// Declare your local variables here
unsigned char i;
// Input/Output Ports initialization
// Port B initialization
// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTB=0x00;
DDRB=0x00;

// Port C initialization
// Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTC=0x00;
DDRC=0x00;

// Port D initialization
// Func7=Out Func6=In Func5=In Func4=Out Func3=In Func2=In Func1=In Func0=In 
// State7=0 State6=T State5=T State4=0 State3=T State2=T State1=T State0=T 
PORTD=0x00;
DDRD=0x90;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: Timer 0 Stopped
TCCR0=0x00;
TCNT0=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: Timer 1 Stopped
// Mode: CTC top=OCR1A
// OC1A output: Discon.
// OC1B output: Discon.
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer 1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: On
// Compare B Match Interrupt: Off
TCCR1A=0x00;
TCCR1B=0x0D;
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x08;
OCR1AL=0x20;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: Timer 2 Stopped
// Mode: Normal top=FFh
// OC2 output: Disconnected
ASSR=0x00;
TCCR2=0x00;
TCNT2=0x00;
OCR2=0x00;

// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
MCUCR=0x00;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=0x10;

// USART initialization
// Communication Parameters: 8 Data, 1 Stop, No Parity
// USART Receiver: On
// USART Transmitter: On
// USART Mode: Asynchronous
// USART Baud rate: 9600
UCSRA=0x00;
UCSRB=0xD8;
UCSRC=0x86;
UBRRH=0x00;
UBRRL=0x5F;

// Analog Comparator initialization
// Analog Comparator: Off
// Analog Comparator Input Capture by Timer/Counter 1: Off
ACSR=0x80;
SFIOR=0x00;
RED_ON

// Watchdog Timer initialization
// Watchdog Timer Prescaler: OSC/256k
#pragma optsize-
WDTCR=0x1C;
WDTCR=0x0C;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

#asm("wdr")
#asm("sei")       // Global enable interrupts
Print(VERSIA_BOOT_LOADER);     

for(i=0;i!=12;i++)
{
 if(FHandContr[i]!=(*(pEEPROM+HND+i)))
  *(pEEPROM+HND+i)=FHandContr[i];
}             
Print("\r\nS/N ");
Print(&FHandContr[6]);

if(check_flash_crc())
{
 Print("\r\nCRC OK\r\n");
 while(tx_counterPC)
      #asm("wdr")
 delay_ms(120);
 #asm("cli")
 /* Enable change of Interrupt Vectors */
 GICR = (1<<IVCE);
 /* Move interrupts to boot Flash section */
 GICR = (1<<IVSEL);
 #asm("rjmp 0x00");                 // Run application code
}
else 
{
 Print("\r\nCRC ERROR\r\n"); 
}

SendAnswerR(BootHere);
   
while (1)                                                          
      { 
      // Place your code here
      #asm("wdr")
      if(rx_counterPC) Receive_PC();
      };
}

void WriteFlash(unsigned int P_address,unsigned char *pData)
{ 
 unsigned char i;
 #asm("wdr")
 #asm("cli");            
 while(EECR & (1<<EEWE));
 #asm("push r0")
 #asm("push r1")
 reg_temp = P_address<<6;
 SPMCR|=(1<<PGERS) | (1<<SPMEN);
 #asm  
     mov r31,r5
     mov r30,r4
     spm
 #endasm
 while(SPMCR&(1<<SPMEN));        // Polled to find out when the CPU is ready for further page updates
 SPMCR|=(1<<RWWSRE)|(1<<SPMEN);  // RWWSB Flag cleared by software when the SPM operation is completed.
 #asm("spm")
 #asm("sei")
 #asm("wdr")
 for (i=0;i!=64;i+=2)
 { 
  #pragma warn-         
  Bufferh=*(pData+i+1);
  Bufferl=*(pData+i);
  reg_temp = i;
  SPMCR =(1<<SPMEN);
  #asm  
     mov r31,r5
     mov r30,r4
     spm
  #endasm
  #pragma warn+
 }
 #asm("cli")
 reg_temp = P_address<<6;;
 SPMCR|=(1<<PGWRT) | (1<<SPMEN);
 #asm  
    mov r31,r5
    mov r30,r4
    spm
 #endasm
 while(SPMCR&(1<<SPMEN));        // Polled to find out when the CPU is ready for further page updates
 SPMCR|=(1<<RWWSRE)|(1<<SPMEN);  // RWWSB Flag cleared by software when the SPM operation is completed.
 #asm("spm");
 #asm("pop r1")
 #asm("pop r0")

 #asm("sei");
}
unsigned char check_flash_crc(void)
{
 unsigned char mas1[64];
 unsigned int i;  
 unsigned int j;
 unsigned int k;
 #asm("wdr") 
 crci.crcw=0;
 while(EECR & (1<<EEWE));
 if((*(pEEPROM+508))>7) return 0;
 k=(*(pEEPROM+508));
 k<<=8; 
 k|=(*(pEEPROM+509));
 for(j=0;j!=k;j++)
 {
  for(i=0;i!=64;i++)
   mas1[i]=*(mas+i+j*64);
  crc2(&mas1[0],&crci.crcw,64);
 }              
 while(EECR & (1<<EEWE));
 if((crci.crcb[1]==(*(pEEPROM+510)))&&(crci.crcb[0]==(*(pEEPROM+511))))
 {
  return 1;
 }
 return 0;
}
/**
 * print with Flash memory string.  
 *
 * @param	*data	a pointer to the string command 
 **/
void Print(flash char *data){
 unsigned char i=0;
 do {     
   #asm("wdr")
   putcharPC(data[i]);
  }while(data[++i]); 
}

/********************************************************
***        OTHER SECTION         ORG 0x760       *******
*********************************************************/

#asm(".CSEG")	
#asm(".ORG 0x760")	
/*****************************************************************************/ 
void mmm(unsigned char *pData)
{
 unsigned char i;
 unsigned char a;
 #asm("wdr") 
 ASCIIToHex(&pData[0],64);
 for(i=0;i!=16;i++)
 {           
  a=pData[i*4];
  pData[i*4]=pData[i*4+3];
  pData[i*4+3]=a;
  a=pData[i*4+1];
  pData[i*4+1]=pData[i*4+2];
  pData[i*4+2]=a;  
 }  
 Gam_cD(&pData[0],&KZU[0],&K[0],8);
 for(i=0;i!=16;i++)
 {           
  a=pData[i*4];
  pData[i*4]=pData[i*4+3];
  pData[i*4+3]=a;
  a=pData[i*4+1];
  pData[i*4+1]=pData[i*4+2];
  pData[i*4+2]=a;  
 }  
}

void ProgramFlashPage(unsigned char *mas)
{           
 unsigned int addr; 
 unsigned char kkk[]="SFLSH0000OK";
 #asm("wdr")
 kkk[5]=mas[0];
 kkk[6]=mas[1];
 kkk[7]=mas[2];
 kkk[8]=mas[3];                 
 ASCIIToHex(&mas[0],2);
 addr=mas[1];
 mmm(&mas[4]);
 WriteFlash(addr,&mas[4]);
 SendAnswerR(kkk);
}

void ProgramEEPROM(unsigned char *mas)
{	    
 unsigned int addr;
 unsigned int lenght;
 unsigned int i;  
 unsigned char kkk[]="SEEPR0000OK";
 #asm("wdr")
 kkk[5]=mas[0];
 kkk[6]=mas[1];
 kkk[7]=mas[2];
 kkk[8]=mas[3];
 ASCIIToHex(&mas[0],4);
 addr=mas[0];
 addr<<=8;
 addr|=mas[1];       
 lenght=mas[2];
 lenght<<=8;
 lenght|=mas[3];
 ASCIIToHex(&mas[8],lenght);
 for(i=0;i!=lenght;i++)
 {                
  *(pEEPROM+addr+i)=mas[i+8];
 }
 if(addr==0x01FC)
 {
    SendAnswerR(kkk);
    while(tx_counterPC);
    while(1);
    //delay_ms(120);
   // #asm("cli") 
    //MCUCR=0x01;                         //interrupts vectors are in the boot sector
    //MCUCR=0x02;
    //#asm("jmp 0x1C00");                 // Run application code
 }                                         
  SendAnswerR(kkk);
}
/**
 * Send answer with Flash memory string.  
 *
 * @param	*data	a pointer to the string command 
 **/
void SendAnswerR(char *data){
 unsigned char i=0;
 unsigned char checkbyte=0;
 #asm("wdr")
 putcharPC('$');
 do {     
   #asm("wdr")
   checkbyte^=data[i];
   putcharPC(data[i]);
  }while(data[++i]); 
   putcharPC('*');
   putcharPC(tascii[(checkbyte&0xF0)>>4]);
   putcharPC(tascii[checkbyte&0x0F]);
   putcharPC('\n');     
}

/**
 * Receive data from PC and shaping packet  
 **/
void Receive_PC(void)
{
 unsigned char data;     
 #asm("wdr")
 data=getcharPC();
  switch (data) {
    case '$': { StartFlagPC=1; counter_PC=0;}
    break;
    case '\n': 
        if(StartFlagPC)
        {
         StartFlagPC=0;
         data=0;                  
         counter_PC=0;
         while((buffer_PC[counter_PC] != '*')&&(buffer_PC[counter_PC] != '\n') )	  
         data^=buffer_PC[counter_PC++];                      
         if(buffer_PC[counter_PC]=='*')
         {      
          if((tascii[(data&0xF0)>>4]==buffer_PC[++counter_PC])&&(tascii[data&0x0F]==buffer_PC[++counter_PC]))

            {
                if(!strncmpf(buffer_PC, "PSIGN", 5))
	            {
	                InitSign(&buffer_PC[5]); 
	            }
	            if(!strncmpf(buffer_PC, "PFLSH", 5))
	            {         
	                ProgramFlashPage(&buffer_PC[5]);
	            }
 	            if(!strncmpf(buffer_PC, "PBLFL", 5))
 	            {         
                    Gost_init(&S[0]);
                    SendAnswerR(BootHere);	    
 	            }
                if(!strncmpf(buffer_PC, "PEEPR", 5))
	            {
	                ProgramEEPROM(&buffer_PC[5]);
                } 
            }
            else
            {
                SendAnswerR(kkkk);
            }
         } 
         else
         {
          ;//SendAnswerR(kkkk);
         }
    }
    break;
    default: 
        if(StartFlagPC) buffer_PC[counter_PC++]=data;  
    };
}                                   

void InitSign(unsigned char *mas)
{
 unsigned char answerC[]="SSIGNOK";
 #asm("wdr")
 ASCIIToHex(&mas[0],8);
 number.byte[3]=mas[0];
 number.byte[2]=mas[1];
 number.byte[1]=mas[2];
 number.byte[0]=mas[3];
 S[0]=number.dword;
 number.byte[3]=mas[4];
 number.byte[2]=mas[5];
 number.byte[1]=mas[6];
 number.byte[0]=mas[7];
 S[1]=number.dword;
 Gost_init(&S[0]);
 SendAnswerR(answerC);
}

/*****************************************************************************/

/**
 * Calculated CRC checksum 
 *
 * @param	*ZAdr	a pointer to the data 
 * @param	*DoAdr	a pointer to the CRC checksum returned
 * @param	lle	amount byte need for calculate CRC checksum
 **/
void crc2(unsigned char *ZAdr,unsigned int *DoAdr,unsigned int lle){
 unsigned char i;
 unsigned int C,NrBat;
 #asm("wdr")
 for(NrBat=0;NrBat!=lle;NrBat++,ZAdr++)
 {          
  C=((*DoAdr>>8)^*ZAdr)<<8;
  for(i=0;i!=8;i++)
   if (C&0x8000)
    C=(C<<1)^0x1021;
   else C=C<<1;
    *DoAdr=C^(*DoAdr<<8);
 }        
}   

void ASCIIToHex(char *mas,unsigned int num)
{
 unsigned char a,b;
 unsigned int i,j;
 #asm("wdr")
 for(i=0,j=0;j!=num;i+=2,j++)
 {             
    a=mas[i]-0x30;
    if(a>9) a-=7;
    a<<=4;
    b=mas[i+1]-0x30;
    if(b>9)b-=7;
    mas[j]=a|b;
 }
}