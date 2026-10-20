#pragma once
#include <stdint.h>

// Реконект до сервера даних: https://info.jaam.net.ua/fusion/api/server-api/
static const uint32_t WS_RECONNECT_MIN_MS = 5000;
static const uint32_t WS_RECONNECT_MAX_MS = 300000;
static const uint32_t WS_RECONNECT_STABLE_MS = 60000;   // коротше за це з'єднання не вважається успішним
static const uint32_t RECONNECT_JITTER_PCT = 20;
static const uint32_t WS_LOST_AFTER_MS = 60000;          // стільки без зв'язку - показуємо "немає зв'язку"

// Повертає затримку (мс) до наступної спроби й готує наступну базу в `next` (подвоює до `maxMs`).
// `stable` - попереднє з'єднання трималося достатньо довго: база скидається до `minMs`; `maxNow` - одразу `maxMs`
// (WebSocket: закриття з кодом 1008). Спільна для WebSocket (5...300 с) і Wi-Fi (2...30 с).
// `jitterPct` - випадкове число 100-J..100+J (передається ззовні, щоб функцію можна було тестувати).
inline uint32_t backoffStep(uint32_t& next, bool stable, bool maxNow, uint32_t jitterPct, uint32_t minMs, uint32_t maxMs) {
    if (maxNow) next = maxMs;
    else if (stable || next < minMs) next = minMs;
    uint32_t delayMs = (uint32_t)((uint64_t)next * jitterPct / 100);
    next = (next * 2 > maxMs) ? maxMs : next * 2;
    return delayMs;
}

inline uint32_t wsBackoffStep(uint32_t& next, bool stable, bool policyClose, uint32_t jitterPct) {
    return backoffStep(next, stable, policyClose, jitterPct, WS_RECONNECT_MIN_MS, WS_RECONNECT_MAX_MS);
}

#ifdef ARDUINO
#include <Arduino.h>
// Випадковий множник у відсотках для backoffStep (100 +- RECONNECT_JITTER_PCT).
inline uint32_t randomJitterPct() { return random(100 - RECONNECT_JITTER_PCT, 101 + RECONNECT_JITTER_PCT); }
#endif
