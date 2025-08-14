#include "stm8s.h"

// 핀 정의
#define RED_PORT    GPIOA
#define RED_PIN     GPIO_PIN_3
#define BLUE_PORT   GPIOC
#define BLUE_PIN    GPIO_PIN_5
#define BTN_PORT    GPIOD
#define BTN_PIN     GPIO_PIN_6

volatile uint32_t millis = 0;    // 1ms 시스템 tick
volatile uint8_t mode = 0;       // 현재 패턴 모드 (0~3)

// 타이머4 인터럽트 (1ms)
INTERRUPT_HANDLER(TIM4_UPD_OVF_IRQHandler, 23) {
    millis++;
    TIM4_ClearITPendingBit(TIM4_IT_UPDATE);
}

uint32_t millis_read(void) {
    uint32_t m;
    DISABLE_INTERRUPTS();
    m = millis;
    ENABLE_INTERRUPTS();
    return m;
}

// 버튼 처리 (디바운스)
void button_check(void) {
    static uint8_t last_state = 1;
    static uint32_t last_time = 0;

    uint8_t state = GPIO_ReadInputPin(BTN_PORT, BTN_PIN);
    uint32_t now = millis_read();

    if (state != last_state) {
        last_time = now; // 변화 시점 기록
        last_state = state;
    }

    if ((now - last_time) > 20) { // 20ms 디바운스
        if (state == 0) { // 버튼 눌림
            static uint8_t pressed = 0;
            if (!pressed) {
                pressed = 1;
                mode++;
                if (mode > 3) mode = 0;
            }
        } else {
            pressed = 0;
        }
    }
}

// 패턴 처리
void pattern_run(void) {
    static uint32_t last_toggle = 0;
    static uint8_t step = 0;
    uint32_t now = millis_read();

    switch (mode) {
        case 0: // 번갈아 켜기 (0.25s)
            if (now - last_toggle >= 250) {
                last_toggle = now;
                if (step == 0) {
                    GPIO_WriteHigh(RED_PORT, RED_PIN);
                    GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
                    step = 1;
                } else {
                    GPIO_WriteLow(RED_PORT, RED_PIN);
                    GPIO_WriteHigh(BLUE_PORT, BLUE_PIN);
                    step = 0;
                }
            }
            break;

        case 1: // RED 3회 → BLUE 3회
            if (now - last_toggle >= 150) {
                last_toggle = now;
                if (step < 6) { // RED 깜빡 3회
                    if (step % 2 == 0) GPIO_WriteHigh(RED_PORT, RED_PIN);
                    else GPIO_WriteLow(RED_PORT, RED_PIN);
                } else if (step < 12) { // BLUE 깜빡 3회
                    if (step % 2 == 0) GPIO_WriteHigh(BLUE_PORT, BLUE_PIN);
                    else GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
                }
                step++;
                if (step >= 12) step = 0;
            }
            break;

        case 2: // 동시에 깜빡 (0.1s)
            if (now - last_toggle >= 100) {
                last_toggle = now;
                if (step == 0) {
                    GPIO_WriteHigh(RED_PORT, RED_PIN);
                    GPIO_WriteHigh(BLUE_PORT, BLUE_PIN);
                    step = 1;
                } else {
                    GPIO_WriteLow(RED_PORT, RED_PIN);
                    GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
                    step = 0;
                }
            }
            break;

        case 3: // All Off
            GPIO_WriteLow(RED_PORT, RED_PIN);
            GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
            step = 0;
            break;
    }
}

void main(void) {
    // 시스템 클럭 16MHz
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);

    // GPIO 초기화
    GPIO_Init(RED_PORT, RED_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(BLUE_PORT, BLUE_PIN, GPIO_MODE_OUT_PP_LOW_FAST);
    GPIO_Init(BTN_PORT, BTN_PIN, GPIO_MODE_IN_PU_NO_IT);

    // TIM4 초기화 (1ms tick)
    TIM4_TimeBaseInit(TIM4_PRESCALER_128, 125 - 1); // 16MHz / 128 = 125kHz → 125 tick = 1ms
    TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
    TIM4_Cmd(ENABLE);
    enableInterrupts();

    while (1) {
        button_check();
        pattern_run();
    }
}
