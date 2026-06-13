#include "debug.h"
#include "stdbool.h"
#include <stdio.h>

uint16_t displayPins[7] = { GPIO_Pin_0, GPIO_Pin_1, GPIO_Pin_2, GPIO_Pin_3, GPIO_Pin_4, GPIO_Pin_7 };

struct DisplayLUT {
    uint16_t first_step;
    uint16_t second_step;
};

struct DisplayLUT first_display_lut[10] = { 
    { 0x7FFE, 0x0 },
    { 0xD0C, 0x0 },
    { 0x3736, 0x604 },
    { 0x1F1E, 0x604 },
    { 0x4D4C, 0x604 },
    { 0x5B5A, 0x604 },
    { 0x7B7A, 0x604 },
    { 0xF0E, 0x0 },
    { 0x7F7E, 0x604 },
    { 0x4F4E, 0x604 },
};

struct DisplayLUT second_display_lut[10] = { 
    { 0x7F01, 0x0 },
    {  0xD73, 0x0 },
    { 0x3701, 0x602 }, // 2
    { 0x1F01, 0x602 }, // 3
    { 0x4D01, 0x602 }, // 4
    { 0x5B01, 0x602 },
    { 0x7B01, 0x602 },
    { 0xF01, 0x0 },
    { 0x7F01, 0x602 },
    { 0x4F01, 0x602 },
};

struct DisplayLUT third_display_lut[10] = { 
    { 0x7C78, 0x7C30 },
    { 0x3430, 0x0 },
    { 0x5C58, 0x5850 },
    { 0x7C78, 0x4840},
    { 0x3430, 0x6860},
    {0x6C68, 0x6860},
    {0x6C68, 0x7870},
    {0x3C38, 0x0},
    {0x7C78, 0x7870},
    {0x3C38, 0x6860}
};

struct DisplayLUT forth_display_lut[10] = { 
    { 0x7C04, 0x3808 },
    { 0x344C, 0x0 },
    { 0x5C04, 0x5828 },
    { 0x7C04, 0x4808},
    { 0x3404, 0x6808},
    {0x6C04, 0x6808},
    {0x6C04, 0x7808},
    {0x3C04, 0x0},
    {0x7C04, 0x7808},
    {0x3C04, 0x6808}
};

struct DisplayLUT special_characters_display[2] = { 
    { 0xA08, 0x0 },
    { 0x2202, 0x0 },
};

// Configures outputs as floating and push pull with a array of 7 pins (8 bit uint).
void configOutputs(uint8_t pp_pins) {
    GPIO_InitTypeDef  GPIO_InitStructure = {0};

    uint16_t pp_pins_address = 0x00;
    uint16_t floating_pins_address = 0x00;

    for (int i = 0; i < 6; i++) {
        bool pin_config = (pp_pins >> i) & 1;

        if (pin_config != 0) {
            pp_pins_address = pp_pins_address | displayPins[i];
        } else {
            floating_pins_address = floating_pins_address | displayPins[i];
        }
    }

    GPIO_InitStructure.GPIO_Pin = floating_pins_address;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

    GPIO_Init(GPIOC, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = pp_pins_address;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    bool last_pin = (pp_pins >> 6) & 1;

    if (last_pin != 0) {
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

        GPIO_Init(GPIOD, &GPIO_InitStructure);
    } else {
        GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_30MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

        GPIO_Init(GPIOD, &GPIO_InitStructure);
    }
}

// 8 Bits for pp pins and 8 bits for pin states
void showNumber(int display, int number) {
    uint16_t display_steps[2] = {0x00, 0x00};

    switch (display) {
        case 1: {
            display_steps[0] = first_display_lut[number].first_step;
            display_steps[1] = first_display_lut[number].second_step;

            break;
        }

        case 2: {
            display_steps[0] = second_display_lut[number].first_step;
            display_steps[1] = second_display_lut[number].second_step;

            break;
        }

        case 3: {
            display_steps[0] = third_display_lut[number].first_step;
            display_steps[1] = third_display_lut[number].second_step;

            break;
        }

        case 4: {
            display_steps[0] = forth_display_lut[number].first_step;
            display_steps[1] = forth_display_lut[number].second_step;

            break;
        }

        case 5: {
            display_steps[0] = special_characters_display[number].first_step;
            display_steps[1] = special_characters_display[number].second_step;

            break;
        }
    }

    // Loop between the two steps
    for (int i = 0; i < 2; i++) {
        // Get High and Low bits (8 bits)
        uint8_t xlow = display_steps[i] & 0xff;
        uint8_t xhigh = (display_steps[i] >> 8);

        if (xhigh == 0 || xlow == 0) {
            continue;
        }

        configOutputs(xhigh);
        
        for (int i = 0; i < 6; i++) {
            bool segment = (xlow >> i) & 1;
            GPIO_WriteBit(GPIOC, displayPins[i], segment);
        }

        bool last_digit = (xlow >> 6) & 1;
        GPIO_WriteBit(GPIOD, GPIO_Pin_0, last_digit);

        Delay_Us(500);
    }
}

void displayTime(uint8_t hours, uint8_t minutes) {
    int hours_first_digit = hours / 10;
    int hours_second_digit = hours - (hours / 10) * 10;

    int minutes_first_digit = minutes / 10;
    int minutes_second_digit = minutes - (minutes / 10) * 10;

    showNumber(1, hours_first_digit);
    showNumber(2, hours_second_digit);
    showNumber(3, minutes_first_digit);
    showNumber(4, minutes_second_digit);
}

void displayWarnings(bool timer_armed, bool realy_overrided) {
    if (timer_armed) {
        showNumber(5, 0);
    }

    if (realy_overrided) {
        showNumber(5, 1);
    }
}

void writeZero() {
    for (int i = 0; i < 6; i++) {
        GPIO_WriteBit(GPIOC, displayPins[i], 0);
    }

    GPIO_WriteBit(GPIOD, GPIO_Pin_0, 0);
}