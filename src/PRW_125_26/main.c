/*****************************************************
This program was produced by the
CodeWizardAVR V1.25.3 Professional
Automatic Program Generator
© Copyright 1998-2007 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : PRW-125-26
Version : 1.0.1
Date    : 17.12.2008
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

eeprom unsigned char *pEEPROM;                              // указатель на EEPROM

flash char tascii[]={"0123456789ABCDEF"};

#define F_VERSIA            "SYVER RFID PRW-125/26 v1.0.1"  // строка версии устройства
#define F_COMMANDNOTSUPPORT "SYCOMMAND NOT SUPPORT"         // команда не поддерживается

#define WIDE_MIN  92
#define WIDE_MAX  129
#define SHORT_MIN 46
#define SHORT_MAX 73

// Declare your global variables here
bit start=0;                                            // флаг старта приема кода карточки
bit cur_bit=0;                                          // значение текущего бита
bit toogle=0;                                           // 
bit StartFlagPC=0;                                      // флаг старта приема пакета от PC

unsigned char count=0;                                  // счетчик количества бит кода карточки
unsigned char seq=0;
unsigned char index=0;
unsigned char CounterCode=0;
unsigned char CounterDelayLedGreen=0;                   // счетчик задержки включения светодиода
unsigned char CounterDelayBeep=0;                       // счетчик задержки включения динамика

unsigned long buffer[4];                                // буфер бит кода карточки (манчестер) 
unsigned long manchester[2];                            // буфер бит кода карточки с битами четности
unsigned long final[2];                                 // буфер бит кода карточки
unsigned char counter_wiegand=0;                        // счетчик бит отправленых по Wiegund протоколу  	
unsigned long wiegand_reg=0;                            // код карточки передаваемый по Wiegund протоколу

unsigned char counter_PC=0;                             // количество принятых байт с компьютера
unsigned char buffer_PC[64];                            // приемный буфер данных с компьютера 
char FHandContr[12];                                    // идентификатор контроллера

void Receive_PC(void);                                  // прием и обработка пакета с компьютера
void SendAnswer(flash char *);                          // посылка строки на компьютер
void SendAnswerF(flash char *);                         // посылка пакета на компьютер с FLASH памяти
void SendAnswerR(char *);                               // посылка пакета на компьютер с RAM памяти
void start_detect(void);                                // начало детектирования
void register_data(void);                               // 
void manchester_to_nrz(void);                           // перевод данных с манчестерского кодирования в NRZ
char check_parity(void);                                // проверка контрольной суммы 

//External Interrupt 1 service routine
interrupt [EXT_INT1] void ext_int1_isr(void)
{
    // Place your code here
    TCCR0=0x00;
    count=TCNT0;
    TCNT0=0x00;
    TCCR0=0x03;
    cur_bit=PIND.3;
    toogle=1;
}

#define RXB8    1
#define TXB8    0
#define UPE     2
#define OVR     3
#define FE      4
#define UDRE    5
#define RXC     7

#define FRAMING_ERROR       (1<<FE)
#define PARITY_ERROR        (1<<UPE)
#define DATA_OVERRUN        (1<<OVR)
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE         (1<<RXC)

// USART Receiver buffer
#define RX_BUFFER_SIZE 128
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
#define TX_BUFFER_SIZE 128
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
    if(counter_wiegand)
    {
        counter_wiegand--;
        if(wiegand_reg&0x02000000)D0=1;  
        else D1=1;
        wiegand_reg<<=1;
    }
    if(CounterCode)CounterCode--; 
    if(CounterDelayLedGreen) CounterDelayLedGreen--;
    if(CounterDelayBeep) CounterDelayBeep--;
}

// Timer 1 output compare B interrupt service routine
interrupt [TIM1_COMPB] void timer1_compb_isr(void)
{
    // Place your code here
    D0=0;
    D1=0;
}


void main(void)
{
// Declare your local variables here
unsigned char i=0;
unsigned char parity;
unsigned long mask_10;

//Input/Output Ports initialization
// Port B initialization
// Func7=In Func6=In Func5=In Func4=In Func3=Out Func2=In Func1=Out Func0=Out 
// State7=T State6=T State5=T State4=T State3=0 State2=T State1=0 State0=0 
PORTB=0x04;
DDRB=0x0B;

// Port C initialization
// Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
PORTC=0x00;
DDRC=0x00;

// Port D initialization
// Func7=Out Func6=Out Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
// State7=0 State6=0 State5=T State4=T State3=P State2=T State1=T State0=T 
PORTD=0x08;
DDRD=0xB0;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 230,400 kHz
TCCR0=0x03;
TCNT0=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 14745,600 kHz
// Mode: CTC top=OCR1A
// OC1A output: Discon.
// OC1B output: Discon.
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer 1 Overflow Interrupt: Off
// Input Capture Interrupt: Off
// Compare A Match Interrupt: On
// Compare B Match Interrupt: On
TCCR1A=0x00;
TCCR1B=0x09;
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x73;
OCR1AL=0x33;
OCR1BH=0x0B;
OCR1BL=0x85;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: 14745,600 kHz
// Mode: CTC top=OCR2
// OC2 output: Toggle on compare match
ASSR=0x00;
TCCR2=0x0;
TCNT2=0x00;
OCR2=0x3A;

// External Interrupt(s) initialization
// INT0: Off
// INT1: On
// INT1 Mode: Any change
GICR|=0x80;
MCUCR=0x04;
GIFR=0x80;

// Timer(s)/Counter(s) Interrupt(s) initialization
TIMSK=0x18;

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

BEEP_ON													// включение динамика для кратковременного сигнала
CounterDelayBeep=100;

pEEPROM=0;
for(i=0;i!=11;i++)										// копирование идентификатора считывателя
 FHandContr[i]=(*(pEEPROM+HND+i));
FHandContr[11]=0;

// Watchdog Timer initialization
// Watchdog Timer Prescaler: OSC/256k
#pragma optsize-
WDTCR=0x1C;
WDTCR=0x0C;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

// Global enable interrupts 
#asm("sei")

SendAnswer(&F_VERSIA[6]);								// выдача версии устройства

while (1)
      {
      // Place your code here
      #asm("wdr")
      // Check buffer overflow with PC
      if(rx_buffer_overflowPC)  {StartFlagPC=0;rx_buffer_overflowPC=0;}
      // Check receive byte with PC
      if(rx_counterPC)          Receive_PC();  
      if(toogle)
      {
        toogle=0;
        if(!start) start_detect();
        else
        {
            if(index<128) register_data();
            else
            {
                manchester_to_nrz();
                if(check_parity())
                {          
                    if(!CounterCode)
                    {   
                        i=final[0]>>24;
                        putchar(tascii[(i&0xF0)>>4]);
                        putchar(tascii[i&0x0F]);
                        i=final[0]>>16;
                        putchar(tascii[(i&0xF0)>>4]);
                        putchar(tascii[i&0x0F]);
                        i=final[0]>>8;
                        putchar(tascii[(i&0xF0)>>4]);
                        putchar(tascii[i&0x0F]);
                        i=final[0];
                        putchar(tascii[(i&0xF0)>>4]);
                        putchar(tascii[i&0x0F]);
                        i=final[1]>>24;
                        putchar(tascii[(i&0xF0)>>4]);
                        putchar(tascii[i&0x0F]);
                        wiegand_reg=((final[0]&0xFFFF)<<8)|(final[1]>>24);                
                        mask_10=0x00800000;
                        parity=0;
                        for(i=0;i!=12;i++)
                        {   
                            if(wiegand_reg&mask_10) parity++;
                            mask_10>>=1;
                        }
                        if(parity&0x01) wiegand_reg|=0x01000000; 
                        parity=0;
                        for(i=0;i!=12;i++)
                        {
                            if(wiegand_reg&mask_10) parity++;
                            mask_10>>=1;
                        }                
                        wiegand_reg<<=1;
                        if(!(parity&0x01)) wiegand_reg|=0x01; 
                        #asm("cli")                 
                        counter_wiegand=26;                 
                        #asm("sei")
                        final[0]=0;
                        final[1]=0;   
                        GREEN_ON
                        CounterDelayLedGreen=100;
                        TCCR2=0x0;
                        BEEP_ON
                        CounterDelayBeep=100;
                    }
                    CounterCode=250;                    
                }
                start=0;
                index=0;
            }
        }
      }
      if(!PINLEDE) GREEN_ON
      else
        if(!CounterDelayLedGreen) RED_ON
      if(!PINBEEPE)
      {
        BEEP_ON
        TCCR2=0x0; 
        CounterCode=250;
      }
      else 
        if(!CounterDelayBeep) {BEEP_OFF  TCCR2=0x19;}
 };
}                
/**
 * прием и обработка данных принятых от компьютера
 **/
void Receive_PC(void)
{
 unsigned char data;
 #asm("wdr")
 data=getcharPC(); 
  switch (data) {
    case '$': { StartFlagPC=1; counter_PC=0; }
    break;
    case '\n': 
        if(StartFlagPC)
        {  
         StartFlagPC=0;
         buffer_PC[counter_PC]=0;
         counter_PC=0;
         data=0;
         while((buffer_PC[counter_PC] != '*')&&(buffer_PC[counter_PC] != '\n')) data^=buffer_PC[counter_PC++];
       	 if((tascii[(data&0xF0)>>4]==buffer_PC[++counter_PC])&&(tascii[(data&0x0F)]==buffer_PC[++counter_PC]))
         {
            if(!strncmpf(buffer_PC, "PCHND", 5))
	        { SendAnswerR(FHandContr); return; }
    		if(!strncmpf(buffer_PC, "PCVER", 5))
            { SendAnswerF(F_VERSIA); return; }
    		if(!strncmpf(buffer_PC, "PBLFL", 5))
		    {        
		        #asm("cli")
                pEEPROM=0;
                *(pEEPROM+CRCEH)=0xFF;
                *(pEEPROM+CRCEL)=0xFF;
 	            while(1);
            }
	        SendAnswerF(F_COMMANDNOTSUPPORT);
	        return; 
		 }   
	}
    break;  
    default: 
        if(StartFlagPC)
        {
         buffer_PC[counter_PC++]=data;
        }  
    };
}   
                
/**
 * отправка строки с Flash памяти
 *
 * @param	*data	указательна строку
 **/
void SendAnswer(flash char *data){
 unsigned char i=0;
  do {     
   #asm("wdr")
   putcharPC(data[i]);
  }while(data[++i]); 
}                               

/**
 * отправка пакета данных (строка с Flash памяти)
 *
 * @param	*data	указатель на строку
 **/
void SendAnswerF(flash char *data){
 unsigned char i=0;
 unsigned char checkbyte=0;
  putcharPC('$'); 
  do {     
   #asm("wdr")
   checkbyte^=data[i];
   putcharPC(data[i]);
  }while(data[++i]); 
   putcharPC('*');
   putcharPC(tascii[(checkbyte&0xF0)>>4]);
   putcharPC(tascii[checkbyte&0x0F]);
   putcharPC('\r');
   putcharPC('\n');     
}
/**
 * отправка пакета данных (строка с RAM памяти)  
 *
 * @param	*data	указатель на строку
 **/
void SendAnswerR(char *data){
 unsigned char i=0;
 unsigned char checkbyte=0;
  putcharPC('$'); 
  do {     
   #asm("wdr")
   checkbyte^=data[i];
   putcharPC(data[i]);
  }while(data[++i]);
  putcharPC('*');
   putcharPC(tascii[(checkbyte&0xF0)>>4]);
   putcharPC(tascii[checkbyte&0x0F]);
   putcharPC('\r');
   putcharPC('\n');     
}
/**
 * старт детектировния данных с карточки
 **/
void start_detect(void)
{      
 switch (seq) {
    case 17:{
                start=1;
                seq=0;
                if((count<WIDE_MAX)&&(count>WIDE_MIN))
                {
                    if(cur_bit)
                    {
                        buffer[index>>5]<<=1;
                        index++;
                    }
                }           
            }
    case 0 :if((count<WIDE_MAX)&&(count>WIDE_MIN)&&(!cur_bit)) seq++;
    break;   
    default:{
                if((count<SHORT_MAX)&&(count>SHORT_MIN)) seq++;
                else seq=0;
            }
    };   
}

/**
 * запись данных во времменый масив бит
 **/
void register_data(void)
{
 if((count<WIDE_MAX)&&(count>WIDE_MIN))
  {
   if(cur_bit)
    {
     buffer[index>>5]<<=1;
     index++;
     buffer[index>>5]<<=1;
     index++;               
    }
   else
    {
     buffer[index>>5]<<=1;
     buffer[index>>5]++;
     index++;  
     buffer[index>>5]<<=1;
     buffer[index>>5]++;             
     index++;
    } 
  }
 if((count<SHORT_MAX)&&(count>SHORT_MIN))
  {
   if(cur_bit)
    {                            
     buffer[index>>5]<<=1;
     index++;
    }
   else
    {
     buffer[index>>5]<<=1;
     buffer[index>>5]++;
     index++;             
    } 
  }
}                                      

/**
 * перевод данных с манчестерского кодирования в NRZ
 **/
void manchester_to_nrz(void)
{
 unsigned char i;
 for(i=0;i<64;i++)
  { 
   if((buffer[i>>4]&(0x40000000>>((i&0x0F)<<1)))==0)      
    {
     manchester[i>>5]<<=1;
     manchester[i>>5]++;
    }
   else
    {
     manchester[i>>5]<<=1;                 
    }
  } 
}                             

/**
 * проверка контрольной суммы 
 **/
char check_parity(void)
{
 bit paritet=0;
 bit paritet1=0;
 bit paritet2=0;
 bit paritet3=0;
 bit paritet4=0;
 bit service=0;
 unsigned char s;
 unsigned char i=0;                
 for(s=1;s!=56;s++)
  {
   if(s<51)
    {
     if(manchester[(s-1)>>5]&(0x80000000>>((s-1)&0x1F)))
      {
       switch(s%5)
        {
         case 0: if(!paritet)return 0;
                 else paritet=0;
                 break; 
         case 1: paritet1=~paritet1; paritet=~paritet; final[i>>5]<<=1; final[i>>5]++; i++; break;
         case 2: paritet2=~paritet2; paritet=~paritet; final[i>>5]<<=1; final[i>>5]++; i++; break;
         case 3: paritet3=~paritet3; paritet=~paritet; final[i>>5]<<=1; final[i>>5]++; i++; break;
         case 4: paritet4=~paritet4; paritet=~paritet; final[i>>5]<<=1; final[i>>5]++; i++; break;
        }
      }
     else
      {
       if(s%5) 
        {
         final[i>>5]<<=1;
         i++; 
        }
       else
        {
         if(paritet)return 0;
        }
      }
    }
   else
    {
     if(manchester[(s-1)>>5]&(0x80000000>>((s-1)&0x1F))) service=1;
     else service=0;
     switch(s)
      {
       case 51: if(paritet1!=service) return 0; break;
       case 52: if(paritet2!=service) return 0; break;
       case 53: if(paritet3!=service) return 0; break;
       case 54: if(paritet4!=service) return 0; break;
       case 55: if(service) return 0; break;
     }
   }
 }
  final[1]<<=24;
  return 1;  
}