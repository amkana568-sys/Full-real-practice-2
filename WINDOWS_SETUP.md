# Запуск проекта в Windows без WSL

Для этого способа не нужны Ubuntu, WSL, Hyper-V или включение виртуальной
машины. Сборка выполняется обычным 64-разрядным компилятором GCC из MSYS2.

## 1. Установка MSYS2

1. Открыть официальный сайт https://www.msys2.org/.
2. Скачайть и установить 64-разрядный установщик MSYS2.
3. После установки открыть именно приложение **MSYS2 UCRT64** из меню «Пуск».
   Не открывайте «MSYS2 MSYS» и не открывайте обычный PowerShell.

## 2. Обновление MSYS2

В окне MSYS2 UCRT64 выполним:

```bash
pacman -Syu
```

Программа попросила закрыть окно, закроем его, снова откроем **MSYS2 UCRT64**
и повторим:

```bash
pacman -Syu
```

## 3. Установка компилятора и инструментов

В том же окне MSYS2 UCRT64 выполним:

```bash
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-curl tar
```

Если появится вопрос о выборе пакетов, нажмите Enter для выбора значений по умолчанию.
Если появится вопрос подтверждения установки, введите `Y` и нажмите Enter.

## 4. Распаковка проекта

Распакуем `practice2.zip` в папку «Загрузки» Windows.
Для пользователя Windows с именем `STRIX` путь в MSYS2 будет таким:

```bash
cd /c/Users/STRIX/Downloads/practice2
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

Для режима нужен доступ к Интернету. Создадим в корне проекта файл
`config_real.toml` с содержимым:

```toml
package = "curl"
repository = "https://dl-cdn.alpinelinux.org/alpine/latest-stable/main/x86_64/APKINDEX.tar.gz"
test_mode = false
ascii_tree = true
max_depth = 3
```

Запустим:

```bash
bash run_windows.sh config_real.toml
```

Команды `curl` и `tar` используются как системные утилиты для скачивания архива
и извлечения текстового APKINDEX. Менеджер пакетов Alpine и сторонние библиотеки
для получения зависимостей не используются.
