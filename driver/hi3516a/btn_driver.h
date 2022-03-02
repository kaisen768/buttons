#ifndef __BTNDRIVER_H__
#define __BTNDRIVER_H__

typedef enum _IONUM_CHIP 
{
    GPIO_CHIP_0  = 0,
    GPIO_CHIP_1  = 1,
    GPIO_CHIP_2  = 2,
    GPIO_CHIP_3  = 3,
    GPIO_CHIP_4  = 4,
    GPIO_CHIP_5  = 5,
    GPIO_CHIP_6  = 6,
    GPIO_CHIP_7  = 7,
    GPIO_CHIP_8  = 8,
    GPIO_CHIP_9  = 9,
    GPIO_CHIP_10 = 10,
    GPIO_CHIP_11 = 11,
    GPIO_CHIP_12 = 12,
    GPIO_CHIP_13 = 13,
    GPIO_CHIP_14 = 14,
    GPIO_CHIP_15 = 15
} IONUM_CHIP;
    
typedef enum _IONUM_BITS 
{
    GPIO_BITS_0 = 0,
    GPIO_BITS_1 = 1,
    GPIO_BITS_2 = 2,
    GPIO_BITS_3 = 3,
    GPIO_BITS_4 = 4,
    GPIO_BITS_5 = 5,
    GPIO_BITS_6 = 6,
    GPIO_BITS_7 = 7
} IONUM_BITS;

#endif
