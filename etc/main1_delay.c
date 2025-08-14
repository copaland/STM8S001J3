#include "stm8s.h"

// 핀 정의
#define RED_PORT    GPIOA
#define RED_PIN     GPIO_PIN_3   // PA3
#define BLUE_PORT   GPIOC
#define BLUE_PIN    GPIO_PIN_5   // PC5
#define BTN_PORT    GPIOD
#define BTN_PIN     GPIO_PIN_6   // PD6

volatile uint8_t mode = 0;  // 0~3 패턴 모드

void delay_ms(uint16_t ms) {
    // 간단한 ms 지연 함수 (CPU 클럭 16MHz 기준)
    for (uint16_t i = 0; i < ms; i++) {
        for (uint16_t j = 0; j < 1600; j++) {
            __asm__("nop");
        }
    }
}

void button_check(void) {
    static uint8_t last_state = 1;
    uint8_t state = GPIO_ReadInputPin(BTN_PORT, BTN_PIN);

    if (state == 0 && last_state == 1) { // 버튼 눌림 (Active Low)
        delay_ms(20); // 디바운스
        if (GPIO_ReadInputPin(BTN_PORT, BTN_PIN) == 0) {
            mode++;
            if (mode > 3) mode = 0;
        }
    }
    last_state = state;
}

void pattern1(void) {
    // RED/BLUE 번갈아 켜기 (0.25초 간격)
    GPIO_WriteHigh(RED_PORT, RED_PIN);
    GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
    delay_ms(250);
    GPIO_WriteLow(RED_PORT, RED_PIN);
    GPIO_WriteHigh(BLUE_PORT, BLUE_PIN);
    delay_ms(250);
}

void pattern2(void) {
    // RED 3번 깜빡 → BLUE 3번 깜빡
    for (uint8_t i = 0; i < 3; i++) {
        GPIO_WriteHigh(RED_PORT, RED_PIN);
        delay_ms(150);
        GPIO_WriteLow(RED_PORT, RED_PIN);
        delay_ms(150);
    }
    for (uint8_t i = 0; i < 3; i++) {
        GPIO_WriteHigh(BLUE_PORT, BLUE_PIN);
        delay_ms(150);
        GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
        delay_ms(150);
    }
}

void pattern3(void) {
    // RED & BLUE 동시에 깜빡 (0.1초 간격)
    GPIO_WriteHigh(RED_PORT, RED_PIN);
    GPIO_WriteHigh(BLUE_PORT, BLUE_PIN);
    delay_ms(100);
    GPIO_WriteLow(RED_PORT, RED_PIN);
    GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
    delay_ms(100);
}

void pattern4(void) {
    // All Off
    GPIO_WriteLow(RED_PORT, RED_PIN);
    GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
    delay_ms(100);
}

void main(void) {
    // 클럭 기본 설정
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1); // 16MHz

    // GPIO 설정
    GPIO_Init(RED_PORT, RED_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(BLUE_PORT, BLUE_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(BTN_PORT, BTN_PIN, GPIO_MODE_IN_PU_NO_IT);

    while (1) {
        button_check(); // 버튼 상태 체크

        switch (mode) {
            case 0: pattern1(); break;
            case 1: pattern2(); break;
            case 2: pattern3(); break;
            case 3: pattern4(); break;
        }
    }
}
