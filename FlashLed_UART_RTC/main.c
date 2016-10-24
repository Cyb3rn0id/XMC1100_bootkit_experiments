/*
 * XMC1100 Bootkit experiment
 * (C)2016 Giovanni Bernardo
 * http://www.settorezero.com
 *
 * This example does:
 * a) alternating led flash using interrupt
 * b) uart tx/rx
 * c) simple use of RTC
 *
 * USIC0 channel 1 is routed on P1.2 and P1.3 that are
 * connected to USB bridge
 *
 * 24 october 2016
 *
 */

#include <xmc_gpio.h>
#include <xmc_uart.h>
#include <xmc_rtc.h>

#define LED1 P0_5
#define LED2 P0_6
#define UART_RX P1_3
#define UART_TX P1_2

#define TICKS_PER_SECOND 1000
#define TICKS_WAIT 1000

const uint8_t message[] = "Hello world!!\n";

XMC_RTC_TIME_t actual_time;

// UART configuration
const XMC_UART_CH_CONFIG_t uart_config =
	{
	.data_bits = 8U,
	.stop_bits = 1U,
	.baudrate = 115200
	};


// RTC Configuration
// 0x7FFF is the default value for prescaler (1:1)
const XMC_RTC_CONFIG_t rtc_conf=
	{
		{
		.seconds=0,
		.minutes=07,
		.hours=23,
		.days=23U,
		.daysofweek=XMC_RTC_WEEKDAY_SUNDAY,
		.month=XMC_RTC_MONTH_OCTOBER,
		.year=2016U
		},
	.prescaler=0x7FFF
	};

// System Timer Interrupt
void SysTick_Handler(void)
	{
    static uint32_t ticks = 0;
    ticks++;
    if (ticks == TICKS_WAIT)
  	  {
    	XMC_GPIO_ToggleOutput(LED1);
    	XMC_GPIO_ToggleOutput(LED2);
    	ticks = 0;

    	// Send actual time on UART
    	XMC_RTC_GetTime(&actual_time);
    	XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.hours/10)+'0');
    	XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.hours%10)+'0');
    	XMC_UART_CH_Transmit(XMC_UART0_CH1, ':');
    	XMC_UART_CH_Transmit(XMC_UART0_CH1,(actual_time.minutes/10)+'0');
    	XMC_UART_CH_Transmit(XMC_UART0_CH1,(actual_time.minutes%10)+'0');
    	XMC_UART_CH_Transmit(XMC_UART0_CH1, ':');
    	XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.seconds/10)+'0');
    	XMC_UART_CH_Transmit(XMC_UART0_CH1, (actual_time.seconds%10)+'0');
    	XMC_UART_CH_Transmit(XMC_UART0_CH1, 10);
  	    XMC_UART_CH_Transmit(XMC_UART0_CH1, 13);
  	  }

	}


int main(void)
{
  // Configure UART
  XMC_UART_CH_Init(XMC_UART0_CH1, &uart_config);
  XMC_UART_CH_SetInputSource(XMC_UART0_CH1, XMC_UART_CH_INPUT_RXD,USIC0_C1_DX0_P1_3);

  // Start UART
  XMC_UART_CH_Start(XMC_UART0_CH1);

  // Configure I/Os
  XMC_GPIO_SetMode(UART_TX, XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7);
  XMC_GPIO_SetMode(UART_RX, XMC_GPIO_MODE_INPUT_TRISTATE);

  XMC_GPIO_SetMode(LED1, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);
  XMC_GPIO_SetMode(LED2, XMC_GPIO_MODE_OUTPUT_PUSH_PULL);

  XMC_GPIO_SetOutputHigh(LED1);
  XMC_GPIO_SetOutputLow(LED2);

  // Configure RTC
  XMC_RTC_Disable();
  XMC_RTC_Init(&rtc_conf);
  XMC_RTC_Enable();
  XMC_RTC_Start();

  // Configure System Tick
  SysTick_Config(SystemCoreClock / TICKS_PER_SECOND);

  uint8_t rxData=0x00; // byte from UART

  while(1)
  	  {
	  // if uart contains data...
	  if((XMC_UART_CH_GetStatusFlag(XMC_UART0_CH1) & (XMC_UART_CH_STATUS_FLAG_RECEIVE_INDICATION | XMC_UART_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION)) != 0)
	  	  {
		  // retrieve data from buffer
		  rxData = XMC_UART_CH_GetReceivedData(XMC_UART0_CH1);
	  	  // clear received byte flags
	  	  XMC_UART_CH_ClearStatusFlag(XMC_UART0_CH1,XMC_UART_CH_STATUS_FLAG_RECEIVE_INDICATION | XMC_UART_CH_STATUS_FLAG_ALTERNATIVE_RECEIVE_INDICATION );
	      // re-send byte on uart
	  	  XMC_UART_CH_Transmit(XMC_UART0_CH1, rxData);
		  }
  	  }
	}
