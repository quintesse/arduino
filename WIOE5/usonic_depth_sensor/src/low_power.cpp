#include <Arduino.h>

#include "low_power.h"

extern "C" void SystemClock_Config(void);

namespace {
RTC_HandleTypeDef rtcWakeHandle;
bool rtcWakeReady = false;

constexpr uint32_t RTC_ASYNC_PREDIV = 0x7Fu;
constexpr uint32_t RTC_SYNC_PREDIV = 0x00FFu;

bool ensureRtcWakeupReady() {
    if (rtcWakeReady) {
        return true;
    }

    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_RTCAPB_CLK_ENABLE();

    RCC_OscInitTypeDef oscInit = {};
    RCC_PeriphCLKInitTypeDef periphClockInit = {};

    oscInit.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    oscInit.LSEState = RCC_LSE_ON;
    oscInit.PLL.PLLState = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&oscInit) != HAL_OK) {
        return false;
    }

    periphClockInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    periphClockInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    if (HAL_RCCEx_PeriphCLKConfig(&periphClockInit) != HAL_OK) {
        return false;
    }

    __HAL_RCC_RTC_ENABLE();

    rtcWakeHandle.Instance = RTC;
    rtcWakeHandle.Init.HourFormat = RTC_HOURFORMAT_24;
    rtcWakeHandle.Init.AsynchPrediv = RTC_ASYNC_PREDIV;
    rtcWakeHandle.Init.SynchPrediv = RTC_SYNC_PREDIV;
    rtcWakeHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
    rtcWakeHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    rtcWakeHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    if (HAL_RTC_Init(&rtcWakeHandle) != HAL_OK) {
        return false;
    }

    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

    rtcWakeReady = true;
    return true;
}
} // namespace

void goToSleep(uint32_t timeoutMs) {
    if (timeoutMs == 0U) {
        return;
    }

    if (!ensureRtcWakeupReady()) {
        delay(timeoutMs);
        return;
    }

    uint32_t wakeSeconds = (timeoutMs + 999U) / 1000U;
    if (wakeSeconds == 0U) {
        wakeSeconds = 1U;
    }

    uint32_t wakeCounter = wakeSeconds - 1U;
    if (wakeCounter > 0xFFFFU) {
        wakeCounter = 0xFFFFU;
    }

    if (HAL_RTCEx_DeactivateWakeUpTimer(&rtcWakeHandle) != HAL_OK) {
        delay(timeoutMs);
        return;
    }

    if (HAL_RTCEx_SetWakeUpTimer_IT(&rtcWakeHandle, wakeCounter, RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 0U) != HAL_OK) {
        delay(timeoutMs);
        return;
    }

    Serial.end();
    HAL_SuspendTick();
    __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    SystemClock_Config();
    HAL_ResumeTick();
    HAL_RTCEx_DeactivateWakeUpTimer(&rtcWakeHandle);
}