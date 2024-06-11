#include <Arduino.h>
#include "Bat.h"
#include "config.h"
ADC_HandleTypeDef hadc;
uint32_t Volt = 0;

/**
 * @brief Initializes the ADC hardware for battery voltage measurement.
 *
 * This function configures the ADC (Analog-to-Digital Converter) settings,
 * including clock prescaler, resolution, sampling time, conversion modes,
 * and more. It enables the GPIOC clock, initializes the ADC instance, and
 * sets up the ADC channel. It also starts ADC calibration.
 */
static void MX_ADC_Init(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    __HAL_RCC_GPIOC_CLK_ENABLE();
    hadc.Instance = ADC1;
    hadc.Init.OversamplingMode = DISABLE;
    hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc.Init.Resolution = ADC_RESOLUTION_12B;
    hadc.Init.SamplingTime = ADC_SAMPLETIME_39CYCLES_5;
    hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
    hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc.Init.ContinuousConvMode = ENABLE;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc.Init.DMAContinuousRequests = DISABLE;
    hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc.Init.LowPowerAutoWait = DISABLE;
    hadc.Init.LowPowerFrequencyMode = DISABLE;
    hadc.Init.LowPowerAutoPowerOff = DISABLE;
    if (HAL_ADC_Init(&hadc) != HAL_OK)
    {
        Error_Handler();
    }
    sConfig.Channel = ADC_CHANNEL_14;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED);
}

/**
 * @brief Initializes the battery voltage measurement system.
 *
 * This function calls the ADC initialization function and starts the ADC
 * for continuous voltage measurement.
 */
void bat_init(void)
{
    MX_ADC_Init();
    HAL_ADC_Start(&hadc);
}

/**
 * @brief Puts the battery voltage measurement system into sleep mode.
 *
 * This function stops the ADC and deinitializes it to reduce power consumption.
 */
void bat_sleep(void)
{
    HAL_ADC_Stop(&hadc);
    HAL_ADC_DeInit(&hadc);
}

/**
 * @brief Continuously measures and updates the battery voltage.
 *
 * This function accumulates ADC readings, averages them over 20 samples, and
 * updates the global variable `Volt` with the averaged value. This process
 * helps in smoothing out noise in the ADC readings.
 */
void bat_loop(void)
{
    static uint32_t vot;
    static uint8_t i;
    if (i < 20)
    {
        i++;
        vot += HAL_ADC_GetValue(&hadc);
    }
    else
    {
        Volt = vot / i;
        vot = 0;
        i = 0;
    }
}