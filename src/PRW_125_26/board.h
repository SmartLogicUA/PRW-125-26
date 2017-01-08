/*
Version : 1.0.1
Date    : 17.12.2008
Author  : T.Drozdovsky
Company : Smart Logic
Comments: 
*/

#ifndef _BOARD_INCLUDED_
#define _BOARD_INCLUDED_

#include <mega8.h>

#define CRCEH           511
#define CRCEL           510
#define HND             496

#define D0              PORTB.0
#define D1              PORTB.1

#define PINLEDE         PINB.2
#define PINBEEPE        PIND.6

#define BEEP_ON         PORTD.5=1;                  // buzer ON on board
#define BEEP_OFF        PORTD.5=0;                  // buzer OFF on board
#define BEEP_TOG        PORTD.5=~PORTD.5;           // buzer ON on board

#define PGREEN          7
#define PRED            4

#define GREEN_ON        {PORTD.PGREEN=1;RED_OFF}    // led red ON on board
#define GREEN_OFF       PORTD.PGREEN=0;             // led red ON on board
#define GREEN_TOG       PORTD.PGREEN=~PORTD.PGREEN; // led red ON on board
#define RED_ON          {PORTD.PRED=1;GREEN_OFF}    // led red ON on board
#define RED_OFF         PORTD.PRED=0;               // led red ON on board
#define RED_TOG         PORTD.PRED=~PORTD.PRED;     // led red ON on board

#define rx_buffer_overflowPC rx_buffer_overflow
#define rx_counterPC    rx_counter
#define tx_counterPC    tx_counter
#define putcharPC       putchar
#define getcharPC       getchar

#endif //_BOARD_INCLUDED_