# STM8S001J3

STM8S001J3를 **VSCode + PlatformIO** 환경에서 **Cosmic C 컴파일러**와 \*\*SPL(Standard Peripheral Library)\*\*로 사용하는 방법

---

## 1\. 사전 준비

1.  **PlatformIO 설치**  
    VSCode의 확장(Extensions)에서 `PlatformIO IDE` 설치.
2.  **Cosmic STM8 컴파일러 설치**

    - Cosmic 공식 사이트: https://www.cosmicsoftware.com/download.php
    - `CXSTM8` 다운로드 → 설치 후 경로 예:

      ```
      javaC:\Program Files (x86)\COSMIC\CXSTM8
      ```

3.  **STM8 SPL 다운로드**

    - ST 사이트: https://www.st.com/en/embedded-software/stsw-stm8069.html
    - `STM8S_StdPeriph_Lib` 압축 해제 후 프로젝트에 포함.

---

## 2\. PlatformIO 프로젝트 생성

1.  **새 프로젝트 만들기**

    - VSCode → `PlatformIO Home` → New Project
    - Board: `Custom`
    - Framework: `spl` (PlatformIO에선 기본 미지원 → 직접 추가)
    - Location: 원하는 폴더.

2.  **platformio.ini 수정**

    ```
    ini[env:stm8s001j3]
    platform = ststm8
    board = stm8s001j3
    framework = spl
    upload_protocol = stlink
    build_type = release

    ; Cosmic C 컴파일러 사용
    compiler = cosmic
    ```

    > ⚠️ `ststm8` 플랫폼은 기본 SDCC를 사용하므로, Cosmic 사용 시 `platformio.ini` 안에 `extra_scripts`로 경로 세팅이 필요합니다.

---

## 3\. Cosmic 컴파일러 경로 설정 (extra_script)

PlatformIO 기본 STM8 빌드는 **SDCC** 기반이라, Cosmic을 사용하려면 툴체인 경로를 수동 지정해야 합니다.

`extra_script.py` 생성:

```markdown
pythonimport os

COSMIC_PATH = r"C:\Program Files (x86)\COSMIC\CXSTM8"
env = DefaultEnvironment()

# Cosmic C 컴파일러 설정

env.Replace(
CC = os.path.join(COSMIC_PATH, "cxstm8.exe"),
AS = os.path.join(COSMIC_PATH, "castm8.exe"),
AR = os.path.join(COSMIC_PATH, "clstm8.exe"),
LINK = os.path.join(COSMIC_PATH, "clnkstm8.exe"),
OBJCOPY = os.path.join(COSMIC_PATH, "chex.exe")
)

# 컴파일 옵션

env.Append(
CCFLAGS = ["+debug", "-pxstm8s001j3", "-l"],
LINKFLAGS = ["-m", "stm8s001j3.lkf"]
)
```

`platformio.ini`에 추가:

```
iniextra_scripts = extra_script.py
```

---

## 4\. SPL 폴더 구조

PlatformIO 프로젝트 구조 예시:

```
css├── include
│   └── stm8s_conf.h
├── lib
│   └── STM8S_StdPeriph_Lib
│       ├── inc
│       └── src
├── src
│   └── main.c
├── platformio.ini
└── extra_script.py
```

---

## 5\. `stm8s_conf.h` 설정

SPL의 기능을 쓰려면 `stm8s_conf.h`를 수정:

```
c#ifndef __STM8S_CONF_H
#define __STM8S_CONF_H

#include "stm8s.h"

/* 사용하려는 모듈 활성화 */
#define USE_FULL_ASSERT    (1)

#endif
```

---

## 6\. 예제 코드 (LED 토글)

**src/main.c**

```
c#include "stm8s.h"

void delay_ms(uint16_t ms) {
    for (uint16_t i = 0; i < ms; i++) {
        for (uint16_t j = 0; j < 1600; j++) {
            __asm__("nop");
        }
    }
}

int main(void) {
    // 클럭 기본값 사용 (HSI)
    CLK_HSIPrescalerConfig(CLK_PRESCALER_HSIDIV1);

    // PD0 출력 설정 (예: LED 연결)
    GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_OUT_PP_LOW_FAST);

    while (1) {
        GPIO_WriteReverse(GPIOD, GPIO_PIN_0);
        delay_ms(500);
    }
}
```

---

## 7\. 빌드 & 업로드

1.  `pio run` → Cosmic C 컴파일러로 빌드.
2.  ST-LINK 연결 후 `pio run -t upload`.

---

✅ **정리**

- 기본 PlatformIO STM8은 SDCC만 지원하므로 Cosmic은 `extra_script.py`로 수동 설정.
- SPL 라이브러리는 `lib` 폴더에 넣고 `stm8s_conf.h`로 사용 모듈 설정.
- Cosmic은 빌드 속도가 빠르고 최적화가 좋아서 실무에서 많이 사용.
- `-pxstm8s001j3` 옵션을 주어 MCU를 지정.
