#include "main.h"

ADC_HandleTypeDef hadc1;

void SystemClock_Config(void);
void Error_Handler(void);

static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);

static uint32_t LDR_Read(void);
static void UpdateLight(uint32_t now);
static void UpdateButtons(uint32_t now);
static void UpdateSignals(uint32_t now);
static void UpdateOutputs(void);

#define DARK_THRESHOLD           1200U
#define BRIGHT_THRESHOLD         1800U
#define LDR_INTERVAL_MS          100U
#define BLINK_INTERVAL_MS        500U
#define DEBOUNCE_MS              50U

static uint32_t ldrValue;
static uint32_t lastLdrTime;
static uint32_t lastBlinkTime;

static uint8_t nightMode;
static uint8_t leftSignalActive;
static uint8_t rightSignalActive;
static uint8_t blinkState;

static GPIO_PinState leftLastRaw = GPIO_PIN_SET;
static GPIO_PinState leftStable = GPIO_PIN_SET;
static uint32_t leftChangeTime;

static GPIO_PinState rightLastRaw = GPIO_PIN_SET;
static GPIO_PinState rightStable = GPIO_PIN_SET;
static uint32_t rightChangeTime;

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_ADC1_Init();

    ldrValue = LDR_Read();
    nightMode = (ldrValue < DARK_THRESHOLD);

    while (1)
    {
        uint32_t now = HAL_GetTick();

        UpdateLight(now);
        UpdateButtons(now);
        UpdateSignals(now);
        UpdateOutputs();
    }
}

static uint32_t LDR_Read(void)
{
    uint32_t value = 4095U;

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
        return value;

    if (HAL_ADC_PollForConversion(&hadc1, 100U) == HAL_OK)
        value = HAL_ADC_GetValue(&hadc1);

    HAL_ADC_Stop(&hadc1);

    return value;
}

static void UpdateLight(uint32_t now)
{
    if ((now - lastLdrTime) < LDR_INTERVAL_MS)
        return;

    lastLdrTime = now;
    ldrValue = LDR_Read();

    if (!nightMode && ldrValue < DARK_THRESHOLD)
        nightMode = 1U;
    else if (nightMode && ldrValue > BRIGHT_THRESHOLD)
        nightMode = 0U;
}

static void UpdateButtons(uint32_t now)
{
    GPIO_PinState leftRaw =
        HAL_GPIO_ReadPin(LEFT_BUTTON_GPIO_Port, LEFT_BUTTON_Pin);

    GPIO_PinState rightRaw =
        HAL_GPIO_ReadPin(RIGHT_BUTTON_GPIO_Port, RIGHT_BUTTON_Pin);

    if (leftRaw != leftLastRaw)
    {
        leftLastRaw = leftRaw;
        leftChangeTime = now;
    }

    if ((now - leftChangeTime) >= DEBOUNCE_MS &&
        leftRaw != leftStable)
    {
        leftStable = leftRaw;

        if (leftStable == GPIO_PIN_RESET)
        {
            if (leftSignalActive)
            {
                leftSignalActive = 0U;
                blinkState = 0U;
            }
            else
            {
                leftSignalActive = 1U;
                rightSignalActive = 0U;
                blinkState = 1U;
                lastBlinkTime = now;
            }
        }
    }

    if (rightRaw != rightLastRaw)
    {
        rightLastRaw = rightRaw;
        rightChangeTime = now;
    }

    if ((now - rightChangeTime) >= DEBOUNCE_MS &&
        rightRaw != rightStable)
    {
        rightStable = rightRaw;

        if (rightStable == GPIO_PIN_RESET)
        {
            if (rightSignalActive)
            {
                rightSignalActive = 0U;
                blinkState = 0U;
            }
            else
            {
                rightSignalActive = 1U;
                leftSignalActive = 0U;
                blinkState = 1U;
                lastBlinkTime = now;
            }
        }
    }
}

static void UpdateSignals(uint32_t now)
{
    if (!leftSignalActive && !rightSignalActive)
    {
        blinkState = 0U;
        return;
    }

    if ((now - lastBlinkTime) >= BLINK_INTERVAL_MS)
    {
        lastBlinkTime = now;
        blinkState ^= 1U;
    }
}

static void UpdateOutputs(void)
{
    GPIO_PinState headlightState =
        nightMode ? GPIO_PIN_SET : GPIO_PIN_RESET;

    GPIO_PinState leftState =
        (leftSignalActive && blinkState)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;

    GPIO_PinState rightState =
        (rightSignalActive && blinkState)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;

    GPIO_PinState buzzerState =
        ((leftSignalActive || rightSignalActive) && blinkState)
        ? GPIO_PIN_SET
        : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(
        LEFT_HEADLIGHT_GPIO_Port,
        LEFT_HEADLIGHT_Pin,
        headlightState
    );

    HAL_GPIO_WritePin(
        RIGHT_HEADLIGHT_GPIO_Port,
        RIGHT_HEADLIGHT_Pin,
        headlightState
    );

    HAL_GPIO_WritePin(
        LEFT_SIGNAL_GPIO_Port,
        LEFT_SIGNAL_Pin,
        leftState
    );

    HAL_GPIO_WritePin(
        RIGHT_SIGNAL_GPIO_Port,
        RIGHT_SIGNAL_Pin,
        rightState
    );

    HAL_GPIO_WritePin(
        BUZZER_GPIO_Port,
        BUZZER_Pin,
        buzzerState
    );
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM = 16U;
    osc.PLL.PLLN = 336U;
    osc.PLL.PLLP = RCC_PLLP_DIV4;
    osc.PLL.PLLQ = 7U;

    if (HAL_RCC_OscConfig(&osc) != HAL_OK)
        Error_Handler();

    clk.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

static void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef config = {0};

    __HAL_RCC_ADC1_CLK_ENABLE();

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1U;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(&hadc1) != HAL_OK)
        Error_Handler();

    config.Channel = ADC_CHANNEL_0;
    config.Rank = 1U;
    config.SamplingTime = ADC_SAMPLETIME_480CYCLES;

    if (HAL_ADC_ConfigChannel(&hadc1, &config) != HAL_OK)
        Error_Handler();
}

static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_GPIO_WritePin(
        GPIOA,
        BUZZER_Pin | LEFT_HEADLIGHT_Pin,
        GPIO_PIN_RESET
    );

    HAL_GPIO_WritePin(
        GPIOB,
        RIGHT_HEADLIGHT_Pin |
        RIGHT_SIGNAL_Pin |
        LEFT_SIGNAL_Pin,
        GPIO_PIN_RESET
    );

    gpio.Pin = BUZZER_Pin | LEFT_HEADLIGHT_Pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin =
        RIGHT_HEADLIGHT_Pin |
        RIGHT_SIGNAL_Pin |
        LEFT_SIGNAL_Pin;

    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);

    gpio.Pin = RIGHT_BUTTON_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(RIGHT_BUTTON_GPIO_Port, &gpio);

    gpio.Pin = LEFT_BUTTON_Pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(LEFT_BUTTON_GPIO_Port, &gpio);
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}

#endif
