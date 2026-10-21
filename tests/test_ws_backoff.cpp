// Хост-тест backoff реконекту (src/JaamBackoff.h). Запуск: pytest tests/test_ws_backoff.py --ip 0 (conftest вимагає --ip)
#include <cassert>
#include "../src/JaamBackoff.h"

int main() {
    uint32_t n = WS_RECONNECT_MIN_MS;
    const uint32_t seq[] = {4, 8, 16, 32, 64, 128, 128, 128};
    for (uint32_t s : seq) assert(wsBackoffStep(n, false, false, 100) == s * 1000);
    assert(wsBackoffStep(n, true, false, 100) == 4000);    // стабільне з'єднання скидає
    assert(wsBackoffStep(n, false, false, 100) == 8000);   // швидкий обрив подвоює
    assert(wsBackoffStep(n, false, true, 100) == 128000); // код 1008
    n = 4000;
    assert(wsBackoffStep(n, false, false, 80) == 3200 && n == 8000);
    n = 40000;
    assert(wsBackoffStep(n, false, false, 120) == 48000);
    n = 40000;
    assert(wsBackoffStep(n, false, false, 80) == 32000);
    n = 2000; // Wi-Fi: 2, 4, 8, 16, 30, 30 с
    const uint32_t wifiSeq[] = {2, 4, 8, 16, 30, 30};
    for (uint32_t s : wifiSeq) assert(backoffStep(n, false, false, 100, 2000, 30000) == s * 1000);
    return 0;
}
