#include <Arduino.h>

#include "battery.h"

namespace {
#ifndef BATTERY_ADC_BITS
#define BATTERY_ADC_BITS 12U
#endif

#ifndef BATTERY_VREF_MV
#define BATTERY_VREF_MV 3000U
#endif

#ifndef BATTERY_DIVIDER_RATIO
#define BATTERY_DIVIDER_RATIO 2.0f
#endif

#ifndef BATTERY_ADC_SETTLE_MS
#define BATTERY_ADC_SETTLE_MS 3U
#endif

#ifndef BATTERY_SAMPLE_COUNT
#define BATTERY_SAMPLE_COUNT 8U
#endif

uint32_t sampleAdcAverage(uint32_t pin, uint8_t sampleCount) {
    uint32_t sum = 0U;
    for (uint8_t i = 0; i < sampleCount; ++i) {
        sum += static_cast<uint32_t>(analogRead(pin));
        // Small spacing between samples to reduce burst noise from switching activity.
        delay(2U);
    }
    return sum / sampleCount;
}

uint16_t estimateRailVoltageMv() {
#if defined(AVREF) && defined(VREFINT_CAL_ADDR)
    // Read internal VREFINT and compare it to factory calibration to infer actual VDDA.
    analogReadResolution(BATTERY_ADC_BITS);
    const uint16_t rawVref = static_cast<uint16_t>(sampleAdcAverage(AVREF, BATTERY_SAMPLE_COUNT));
    if (rawVref == 0U) {
        return 0U;
    }

    const uint16_t vrefintCal = *(reinterpret_cast<const uint16_t *>(VREFINT_CAL_ADDR));
    if (vrefintCal == 0U || vrefintCal == 0xFFFFU) {
        return 0U;
    }

    const uint32_t railMv = (static_cast<uint32_t>(BATTERY_VREF_MV) * static_cast<uint32_t>(vrefintCal)) / rawVref;
    return static_cast<uint16_t>(railMv);
#else
    return 0U;
#endif
}
} // namespace

uint16_t measureBatteryVoltageMv() {
    const uint16_t railMv = estimateRailVoltageMv();

#if defined(BATTERY_ADC_PIN)
    // Optional hardware path: an external divider senses battery before regulator.
    if (railMv == 0U) {
        return 0U;
    }

    pinMode(BATTERY_ADC_PIN, INPUT_ANALOG);
    delay(BATTERY_ADC_SETTLE_MS);

    analogReadResolution(BATTERY_ADC_BITS);
    const uint32_t adcFullScale = (1UL << BATTERY_ADC_BITS) - 1UL;
    const uint16_t rawBattery = static_cast<uint16_t>(sampleAdcAverage(BATTERY_ADC_PIN, BATTERY_SAMPLE_COUNT));
    const float adcMv = (static_cast<float>(rawBattery) * static_cast<float>(railMv)) / static_cast<float>(adcFullScale);
    return static_cast<uint16_t>(adcMv * BATTERY_DIVIDER_RATIO);
#else
    // Default hardware path: report regulated rail voltage when no battery sense divider exists.
    return railMv;
#endif
}
