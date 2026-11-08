# Tasks

## 1. Backoff state and constants

- [x] 1.1 Додати константи `WS_RECONNECT_*` і чисту функцію `wsBackoffStep` у новий `src/JaamBackoff.h` (без Arduino-залежностей для хост-тесту) і змінні `wsNextConnectAt`, `wsReconnectDelayMs`, `wsConnectedAt` у `src/JaamFusion.cpp`; перевірити: `pio run` збирається.
- [x] 1.2 Реалізувати `scheduleReconnect(bool policyClose)` (подвоєння, cap 128 с, правило 60 с, jitter ±20%); перевірити: `pytest tests/test_ws_backoff.py --ip 0` (assert-перевірка послідовності 4,8,16,…,128 і меж jitter, скидання після ≥ 60 с).

## 2. Wiring

- [x] 2.1 У `websocketProcess()` підключатися лише при `millis() >= wsNextConnectAt`; перевірити: в логах при недоступному сервері інтервали між `[WEBSOCKET] connection start` зростають.
- [x] 2.2 У `onEventsCallback` (ConnectionClosed) викликати `scheduleReconnect` (1008 → максимум) і прибрати `delay(500)`; у `socketConnect()` викликати `scheduleReconnect` при невдачі та зберігати `wsConnectedAt` при успіху; перевірити: закриття з кодом 1008 дає затримку ≈ 128 с. (1008 перевірено лише хост-тестом, на пристрої не перевірялось)
- [x] 2.3 У `handleReconnectWebsocket()` скидати backoff (`wsNextConnectAt = 0`, база 4 с); перевірити: зміна `WS_SERVER_HOST` під час очікування підключає одразу.

## 3. Connection-lost indication

- [x] 3.1 Показ на мапі й дисплеї після 60 с без з'єднання (`wsLostShown`, `WS_LOST_AFTER_MS`), анімація без обмеження в часі; перевірити: на неправильному хості в логу `no connection for 60 s`, вогник біжить довше 3 хв і мапа не стає зеленою, на екрані «недоступний».
- [x] 3.2 `RUNNING_LIGHT` за поточною яскравістю мапи (вогник і фон); перевірити: зміна яскравості під час індикації змінює яскравість вогника. (на пристрої не перевірялось)

## 4. Verification

- [x] 4.1 Прогнати на пристрої/симуляції сценарії: сервер недоступний, обрив стабільного з'єднання, обрив < 60 с, втрата Wi-Fi; перевірити за логами відповідність `specs/websocket-reconnect/spec.md`.

## 5. Статус реконекту на дисплеї

- [x] 5.1 Показувати на екрані номер спроби і час до наступної (лічильник `wsAttempt`, скидається після успіху та ручної зміни адреси). Перевірка: `pio run -e firmware` збирається; на пристрої з неправильним хостом після 60 с екран показує `спроба N` і зворотний відлік.
