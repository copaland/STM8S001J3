#include "stm8s.h"
//(VSCode + PlatformIO + STM8S001J3 + SPL + Cosmic C)
// 핀 정의
#define RED_PORT    GPIOA
#define RED_PIN     GPIO_PIN_3
#define BLUE_PORT   GPIOC
#define BLUE_PIN    GPIO_PIN_5
#define BTN_PORT    GPIOD
#define BTN_PIN     GPIO_PIN_6

volatile uint32_t millis = 0;    // 1ms 시스템 tick
volatile uint8_t mode = 0;       // 현재 패턴 모드 (0~3)

// 패턴 설정 구조체
typedef struct {
    uint16_t on_time_ms;    // LED ON 시간
    uint16_t off_time_ms;   // LED OFF 시간
    uint8_t  blink_count;   // 깜빡임 횟수 (0이면 무한)
} PatternConfig;

// 모드별 설정
PatternConfig patterns[] = {
    {250, 250, 0},  // 패턴1: 번갈아 켜기, 무한, 0.25s 간격
    {150, 150, 3},  // 패턴2: RED 3회 → BLUE 3회
    {100, 100, 0},  // 패턴3: 동시에 깜빡, 무한, 0.1s 간격
    {0,   0,   0}   // 패턴4: All Off
};

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
        last_time = now;
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
    static uint8_t blink_counter = 0;
    uint32_t now = millis_read();

    PatternConfig cfg = patterns[mode];

    switch (mode) {
        case 0: // 번갈아 켜기
            if (now - last_toggle >= (step ? cfg.off_time_ms : cfg.on_time_ms)) {
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

        case 1: // RED n회 → BLUE n회
            if (now - last_toggle >= (step % 2 ? cfg.off_time_ms : cfg.on_time_ms)) {
                last_toggle = now;

                if (blink_counter < cfg.blink_count * 2) { // RED 깜빡
                    if (step % 2 == 0) GPIO_WriteHigh(RED_PORT, RED_PIN);
                    else GPIO_WriteLow(RED_PORT, RED_PIN);
                } else if (blink_counter < cfg.blink_count * 4) { // BLUE 깜빡
                    if (step % 2 == 0) GPIO_WriteHigh(BLUE_PORT, BLUE_PIN);
                    else GPIO_WriteLow(BLUE_PORT, BLUE_PIN);
                }
                
                step++;
                if (step >= 2) { // 한 번 ON/OFF 사이클 끝
                    step = 0;
                    blink_counter++;
                    if (blink_counter >= cfg.blink_count * 4) {
                        blink_counter = 0; // 다시 RED부터
                    }
                }
            }
            break;

        case 2: // 동시에 깜빡
            if (now - last_toggle >= (step ? cfg.off_time_ms : cfg.on_time_ms)) {
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
            blink_counter = 0;
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
    TIM4_TimeBaseInit(TIM4_PRESCALER_128, 125 - 1);
    TIM4_ITConfig(TIM4_IT_UPDATE, ENABLE);
    TIM4_Cmd(ENABLE);
    enableInterrupts();

    while (1) {
        button_check();
        pattern_run();
    }
}
