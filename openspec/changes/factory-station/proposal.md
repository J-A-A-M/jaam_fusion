# Proposal

## Why

Для jaam_fusion немає станції: прошивка йде через браузерний flasher, а запис у реєстр адмінки заводять вручну. Основна
зміна (`jaam_server`, `device-order-autoregistration`) автоматизує внесення даних замовлення; fusion-плати мають іти тим
самим шляхом, що й touch: за номером замовлення, з тестом і підтвердженням.

## What Changes

- Нова станція `tools/factory/factory_flash.py` (потік як у jaam_touch): стирання -> MAC -> chip_id (верхній регістр) ->
  `POST /api/factory/register` -> тестова прошивка -> оператор -> робоча прошивка -> `complete` (БРАК -> `release`).
- Робоча прошивка: останній реліз GitHub, asset за родиною чипа (`JAAM`, `JAAM_s3`, `JAAM_c3`), bootloader/partitions
  - ті самі, що в браузерному flasher (шлях задається прапорцем).
- Мінімальна тестова прошивка `env:factory_test` (`src/factory_test/`): чип/flash, Wi-Fi станції, WebSocket з
  `chip_id:`/`firmware:`, підтвердження оператором; `FT:*` рядки в serial, як у touch.
- Станція обслуговує лише тип `JAAM3.3` (`JAAM2`/`JAAM1` - інші плати, поза межами).
- Токен адміна станції - `JAAM_FACTORY_TOKEN_PROD`/`_DEV`, `--stage prod|dev` обов'язковий.

## Capabilities

### New Capabilities
- `fusion-factory-station`: станція й тестова прошивка jaam_fusion.

## Impact

- `tools/factory/*`, `src/factory_test/*`, `platformio.ini`, README/CONTROLS_GUIDE; бойова прошивка не змінюється
  (NVS-секрету немає).
- Склад тесту (датчики, дисплей, стрічка) - відкрите питання з `device-order-autoregistration/design.md`.
