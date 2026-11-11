# Запуск проекта в Windows без WSL

Для этого способа не нужны Ubuntu, WSL, Hyper-V или включение виртуальной
машины. Сборка выполняется обычным 64-разрядным компилятором GCC из MSYS2.

## 1. Установка MSYS2

1. Откройте официальный сайт https://www.msys2.org/.
2. Скачайте и установите 64-разрядный установщик MSYS2.
3. После установки откройте именно приложение **MSYS2 UCRT64** из меню «Пуск».
   Не открывайте «MSYS2 MSYS» и не открывайте обычный PowerShell.

## 2. Обновление MSYS2

В окне MSYS2 UCRT64 выполните:

```bash
pacman -Syu
```

Если программа попросит закрыть окно, закройте его, снова откройте **MSYS2 UCRT64**
и повторите:

```bash
pacman -Syu
```

## 3. Установка компилятора и инструментов

В том же окне MSYS2 UCRT64 выполните:

```bash
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-curl tar
```

Если появится вопрос о выборе пакетов, нажмите Enter для выбора значений по умолчанию.
Если появится вопрос подтверждения установки, введите `Y` и нажмите Enter.

## 4. Распаковка проекта

Распакуйте `variant23_dependency_visualizer.zip` в папку «Загрузки» Windows.
Для пользователя Windows с именем `STRIX` путь в MSYS2 будет таким:

```bash
cd /c/Users/STRIX/Downloads/variant23_dependency_visualizer
```

Если папка распакована в другое место, измените путь. В MSYS2 диск `C:` записывается
как `/c/`, а обратные косые черты Windows заменяются обычными `/`.

## 5. Сборка, тесты и запуск

Из корня проекта выполните одну команду:

```bash
bash run_windows.sh
```

Скрипт соберёт программу, запустит автоматические тесты и выполнит пример из
`config.toml`. Для трёх демонстрационных пакетов запустите:

```bash
bash run_windows.sh examples/config_A.toml
bash run_windows.sh examples/config_X.toml
bash run_windows.sh examples/config_P.toml
```

Программа создаёт `graph.puml` и `graph.svg` в корне проекта. Чтобы открыть SVG
в браузере Windows, выполните:

```bash
explorer.exe graph.svg
```

## 6. Настоящий индекс Alpine APK

Для реального режима нужен доступ к Интернету. Создайте в корне проекта файл
`config_real.toml` с содержимым:

```toml
package = "curl"
repository = "https://dl-cdn.alpinelinux.org/alpine/v3.19/main/x86_64/APKINDEX.tar.gz"
test_mode = false
ascii_tree = true
max_depth = 3
```

Запустите:

```bash
bash run_windows.sh config_real.toml
```

Команды `curl` и `tar` используются как системные утилиты для скачивания архива
и извлечения текстового APKINDEX. Менеджер пакетов Alpine и сторонние библиотеки
для получения зависимостей не используются.

## Возможные проблемы

- `bash: cmake: command not found` — повторите установку пакета CMake из шага 3.
- `ninja: command not found` — повторите установку пакета Ninja из шага 3.
- `g++: command not found` — повторите установку пакета GCC из шага 3.
- Не удаётся перейти в каталог — проверьте, где распакован архив, и исправьте путь
  в команде `cd`.
- Не запускается `config_real.toml` — проверьте Интернет и доступность URL.
  Для демонстрации алгоритмов используйте тестовые конфигурации A, X и P.
