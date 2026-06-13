#include "debug.h"
#include "stdbool.h"
#include <stdio.h>
#include <string.h>

#include "display.h"
#include "rtc_lib.h"

// Display pins: PC0-4 & PC7 + PD0
// Button Pins: PD2 & 3
// RTC Pins: PC5-6
// Relay Pin: PD4

uint8_t start_timer[2] = { 5, 45 }; // Time that the timer enables the relay
uint8_t stop_timer[2] = { 5, 48 }; // Time that the timer disables the relay

volatile int currentNumber = 0; 

volatile bool relay_overrided = false;

volatile bool timer_armed = false;
bool timer_configured = false;

char receivedTime[12] = {};
int receivedChars = 0;

int debounce_time = 200;
volatile bool debounce_active = false;
volatile int debounce_counter = 0;

/*********************************************************************
 * @fn      USARTx_CFG
 *
 * @brief   Initializes the USART2 & USART3 peripheral.
 *
 * @return  none
 */
void USARTx_CFG(void)
{
    GPIO_InitTypeDef  GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1, ENABLE);

    /* USART1 TX-->D.5   RX-->D.6 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);
}

// TODO: Make this work lol
void setTimeUSART() {
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) != RESET) {
        if (receivedChars > 11) {
            printf("%s", receivedTime);
            receivedChars = 0;
            memset(receivedTime, 0, sizeof(receivedTime));
        }

        receivedTime[receivedChars] = USART_ReceiveData(USART1);

        if (receivedTime[receivedChars] == 's') {
            writeRTC(30, 47, 22, 13, 10, 24);
        }

        receivedChars++;
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    USART_Printf_Init(115200);
    USARTx_CFG();

    initRTC();
    // 303012131024

    GPIO_InitTypeDef  GPIO_InitStructure = {0};

    //Inits the displays in Open Drain (Floating)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Inits the button pins and its interrupts

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;

    GPIO_Init(GPIOD, &GPIO_InitStructure);

    EXTI_InitTypeDef EXTI_InitStructure = { 0 };
    NVIC_InitTypeDef NVIC_InitStructure = { 0 };

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource2); // PD2
    EXTI_InitStructure.EXTI_Line = EXTI_Line2;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;

    EXTI_Init(&EXTI_InitStructure);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOD, GPIO_PinSource3); // PD3
    EXTI_InitStructure.EXTI_Line = EXTI_Line3;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI7_0_IRQn;
    NVIC_InitStructure .NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

    GPIO_Init(GPIOD, &GPIO_InitStructure);

    while(1)
    {
        setTimeUSART();

        displayTime(time_buf[2], time_buf[1]);
        displayWarnings(timer_armed, relay_overrided);

        if (timer_armed) {
            if (relay_overrided) {
                relay_overrided = false;
                GPIO_WriteBit(GPIOD, GPIO_Pin_4, 0);
            }
            

            printf("Time: %02d:%02d:%02d\r\n",  
            time_buf[2], time_buf[1], time_buf[0]);

            if (start_timer[0] == time_buf[2] && start_timer[1] == time_buf[1]) {
                GPIO_WriteBit(GPIOD, GPIO_Pin_4, 1);
            }

            if (stop_timer[0] == time_buf[2] && stop_timer[1] == time_buf[1]) {
                GPIO_WriteBit(GPIOD, GPIO_Pin_4, 0);
                timer_armed = false;
            }
        } else {
            if (relay_overrided) {
                GPIO_WriteBit(GPIOD, GPIO_Pin_4, 1);
            } else {
                GPIO_WriteBit(GPIOD, GPIO_Pin_4, 0);
            }
        }

        if (debounce_active) {
            debounce_counter--;
            if (debounce_counter == 0) {
                debounce_active = false;
            }
        }
    }
}

void EXTI7_0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI7_0_IRQHandler(void) {
    

    // PD2 Interrupt
    if (EXTI_GetITStatus(EXTI_Line2) != RESET) {
        if (debounce_active) {
            EXTI_ClearITPendingBit(EXTI_Line2);
            return;
        }

        timer_armed = !timer_armed;

        debounce_active = true;
        debounce_counter = debounce_time;

        EXTI_ClearITPendingBit(EXTI_Line2);
    }

    if (EXTI_GetITStatus(EXTI_Line3) != RESET) {
        if (debounce_active) {
            EXTI_ClearITPendingBit(EXTI_Line3);
            return;
        }

        if (!timer_armed) {
            relay_overrided = !relay_overrided;
        }

        debounce_active = true;
        debounce_counter = debounce_time;

        EXTI_ClearITPendingBit(EXTI_Line3);
    }
}