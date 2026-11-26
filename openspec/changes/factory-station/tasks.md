# Tasks

- [ ] 1. `tools/factory/factory_flash.py`: розбір аргументів (`--stage`, `--order-number`, `--no-order`, `--port`), MAC -> chip_id (верхній регістр), `register`, звіт; перевірка: pytest з фейковою адмінкою
- [ ] 2. Вибір робочої прошивки за родиною чипа з релізу GitHub, кеш, прапорець `--app-build-dir`; перевірка: тест вибору asset для ESP32/S3/C3
- [ ] 3. `env:factory_test` і `src/factory_test/` (чип, flash, Wi-Fi, WS-перевірка, підтвердження оператором, `FT:*`); перевірка: збірка `pio run -e factory_test`, прогін на платі
- [ ] 4. `complete`/`release`, БРАК, переривання; перевірка: тести гілок
- [ ] 5. README станції; перевірка: прочитати
