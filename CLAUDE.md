# nch-lighting-control-controller

Bare-metal прошивка для **STM32F030C6Tx** (Cortex-M0). Только CMSIS и прямой доступ
к регистрам — HAL не используется. Сборка через CMake + presets.

## Проверка компиляции

Тулчейн `arm-none-eabi-*`, `cmake` и `ninja` уже в PATH (пути на плагины CubeIDE
прописаны в переменных окружения Windows) — отдельно добавлять их не нужно.

```bash
cmake --preset Debug
cmake --build --preset Debug
```

Успешная сборка `Debug` = код компилируется и линкуется. Этого достаточно для проверки.
Артефакты (`.elf`/`.hex`/`.bin`/`.map`) появляются в `Debug/`.

### Замечания

- Только configure (без билда), если нужна быстрая проверка настроек:
  `cmake --preset Debug`
- Чистая пересборка: удалить каталог `Debug/` (или `Release/`) и запустить заново.
- Меняешь пути include/дефайны/флаги — правь `CMakeLists.txt`, не пресеты.

## Структура

- `Src/` — исходники (`.c`/`.cpp`), кроме `sysmem.c` (исключён).
- `Startup/startup_stm32f030c6tx.s`, `STM32F030C6TX_FLASH.ld` — старт и линкер-скрипт.
- `Inc/`, `common/`, `uart_logger/`, `EncButton/src/core/` — заголовки.
- CMSIS-драйверы — внешний репозиторий, путь в `CMSIS_F0_PATH`. Обязателен
  (по умолчанию пустой, конфигурирование падает с `FATAL_ERROR`, если не задан).
  Задаётся через `-DCMSIS_F0_PATH=...` либо (рекомендуется) через
  `CMakeUserPresets.json` — не коммитится, каждый разработчик заводит свой
  (пример: `Debug`/`Release` пресеты наследуются от скрытых `Debug-base`/
  `Release-base` из `CMakePresets.json` и добавляют `CMSIS_F0_PATH`).
