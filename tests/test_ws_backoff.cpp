// Хост-тест backoff реконекту (src/JaamWsBackoff.h). Запуск: pytest tests/test_ws_backoff.py --ip 0 (conftest вимагає --ip)
#include <cassert>
#include "../src/JaamWsBackoff.h"

int main() {
    uint32_t n = WS_RECONNECT_MIN_MS;
    const uint32_t seq[] = {5, 10, 20, 40, 80, 160, 300, 300, 300};
    for (uint32_t s : seq) assert(wsBackoffStep(n, false, false, 100) == s * 1000);
    assert(wsBackoffStep(n, true, false, 100) == 5000);    // стабільне з'єднання скидає
    assert(wsBackoffStep(n, false, false, 100) == 10000);  // швидкий обрив подвоює
    assert(wsBackoffStep(n, false, true, 100) == 300000);  // код 1008
    n = 5000;
    assert(wsBackoffStep(n, false, false, 80) == 4000 && n == 10000);
    n = 40000;
    assert(wsBackoffStep(n, false, false, 120) == 48000);
    n = 40000;
    assert(wsBackoffStep(n, false, false, 80) == 32000);
    n = 2000; // Wi-Fi: 2, 4, 8, 16, 30, 30 с
    const uint32_t wifiSeq[] = {2, 4, 8, 16, 30, 30};
    for (uint32_t s : wifiSeq) assert(backoffStep(n, false, false, 100, 2000, 30000) == s * 1000);
    return 0;
}
