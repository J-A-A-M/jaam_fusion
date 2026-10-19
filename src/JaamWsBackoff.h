#pragma once
#include <stdint.h>

// Реконект до сервера даних: https://info.jaam.net.ua/fusion/api/server-api/
static const uint32_t WS_RECONNECT_MIN_MS = 5000;
static const uint32_t WS_RECONNECT_MAX_MS = 300000;
static const uint32_t WS_RECONNECT_STABLE_MS = 60000;   // коротше за це з'єднання не вважається успішним
static const uint32_t WS_RECONNECT_JITTER_PCT = 20;

// Повертає затримку (мс) до наступної спроби й готує наступну базу в `next`.
// `stable` - попереднє з'єднання трималося >= WS_RECONNECT_STABLE_MS; `policyClose` - закриття з кодом 1008.
// `jitterPct` - випадкове число 100-J..100+J (передається ззовні, щоб функцію можна було тестувати).
inline uint32_t wsBackoffStep(uint32_t& next, bool stable, bool policyClose, uint32_t jitterPct) {
    if (policyClose) next = WS_RECONNECT_MAX_MS;
    else if (stable || next < WS_RECONNECT_MIN_MS) next = WS_RECONNECT_MIN_MS;
    uint32_t delayMs = (uint32_t)((uint64_t)next * jitterPct / 100);
    next = (next * 2 > WS_RECONNECT_MAX_MS) ? WS_RECONNECT_MAX_MS : next * 2;
    return delayMs;
}
