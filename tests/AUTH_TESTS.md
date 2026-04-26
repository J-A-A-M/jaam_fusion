# Тести веб-автентифікації

Інтеграційні тести для системи веб-автентифікації JAAM Fusion.

Тести запускаються проти живого пристрою через HTTP і самостійно керують станом автентифікації — визначають поточний стан, тимчасово змінюють його на час виконання класу тестів, а після повертають до початкового.

## Вимоги

```
pip install pytest requests
```

## Запуск

```bash
pytest tests/test_web_auth.py --ip <ip-пристрою>
pytest tests/test_web_auth.py --ip <ip-пристрою> --login admin --password secret
```

| Параметр | За замовчуванням | Опис |
|---|---|---|
| `--ip` | *(обов'язковий)* | IP-адреса пристрою |
| `--login` | `admin` | Логін для входу |
| `--password` | `admin` | Пароль для входу |

Перед запуском тестів виконується пінг пристрою. Якщо пристрій недоступний — весь набір тестів пропускається. Стан auth на пристрої не має значення — фікстури визначають його автоматично і відновлюють після тестів.

## Класи тестів

### `TestAuthDisabled`

Перевіряє, що всі ендпоінти доступні без автентифікації, коли auth вимкнено.

| Тест | Що перевіряється |
|---|---|
| `test_root_accessible` | `GET /` повертає 200 |
| `test_system_info_accessible` | `GET /system-info` повертає 200 |
| `test_alerts_info_accessible` | `GET /alerts-info` повертає 200 |
| `test_login_page_accessible` | `GET /login` повертає 200 |
| `test_static_assets_accessible` | `/styles.css` і `/scripts.js` повертають 200 |

### `TestAuthEnabled`

Перевіряє захист ендпоінтів та всі сценарії входу/виходу, коли auth увімкнено.

| Тест | Що перевіряється |
|---|---|
| `test_root_redirects_to_login` | `GET /` без сесії → 302 на `/login` |
| `test_protected_endpoints_redirect_without_session` | Захищені ендпоінти → 302 на `/login` |
| `test_login_success` | Правильні дані → 302 без помилки |
| `test_login_success_grants_access` | Після входу `GET /` повертає 200 |
| `test_login_wrong_password` | Неправильний пароль → `error=1` |
| `test_login_wrong_login` | Неправильний логін → `error=1` |
| `test_login_empty_submitted_password` | Порожній пароль → `error=1` |
| `test_login_invalid_recovery_token` | Невірний токен відновлення → `error=2` |
| `test_login_empty_recovery_token_falls_through_to_login` | Порожній recovery-токен → сервер ігнорує recovery і перевіряє логін/пароль → `error=1` |
| `test_logout_clears_session` | Після виходу сесія більше не дає доступу |
| `test_login_page_public` | `GET /login` повертає 200 без сесії |
| `test_static_assets_public` | Статичні файли повертають 200 без сесії |

## Керування станом пристрою

Фікстури `ensure_auth_disabled` і `ensure_auth_enabled` (scope=`class`) автоматично налаштовують стан auth:

- **`ensure_auth_disabled`** — якщо auth увімкнено, вимикає його перед класом і відновлює після.
- **`ensure_auth_enabled`** — якщо auth вимкнено, встановлює тестові облікові дані і вмикає auth перед класом, вимикає після. Якщо auth вже увімкнено — використовує передані `--login`/`--password`.

Тести запускаються незалежно від початкового стану auth на пристрої — він завжди відновлюється після виконання.

## Коди помилок

| Код | Значення |
|---|---|
| `error=1` | Невірний логін або пароль |
| `error=2` | Невірний токен відновлення |
