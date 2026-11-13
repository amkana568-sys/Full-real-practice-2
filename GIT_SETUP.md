# Как самостоятельно опубликовать проект в GitHub

Инструкция рассчитана на MSYS2 UCRT64 в Windows. Проект уже содержит папку
`.git` и историю разработки. Повторно выполнять `git init` не нужно.

## 1. Распаковать проект

Распакуйте архив `variant23_dependency_visualizer_FINAL.zip` в удобную папку.
Например:

`C:\Users\ИМЯ\Desktop\variant23_dependency_visualizer_FINAL`

Откройте MSYS2 UCRT64 и перейдите в эту папку. Пример для пользователя STRIX:

```bash
cd "/c/Users/STRIX/Desktop/variant23_dependency_visualizer_FINAL"
```

Укажите фактический путь, в который вы распаковали архив.

## 2. Проверить Git и настроить автора

Проверьте наличие Git:

```bash
git --version
```

Если Git не найден, установите его в MSYS2 UCRT64:

```bash
pacman -S --needed git
```

Укажите свои реальные данные. Email должен быть связан с аккаунтом GitHub
(можно взять адрес `noreply` в настройках GitHub):

```bash
git config user.name "Имя Фамилия"
git config user.email "ВАШ_EMAIL"
```

Проверьте настройки:

```bash
git config user.name
git config user.email
```

## 3. Заменить технического автора в уже созданных коммитах

В подготовленной истории первоначально указан технический автор
`Project Author <project-author@example.invalid>`. Перед первой публикацией
замените его на свои данные. Подставьте одинаковые реальные имя и email
вместо `ВАШЕ ИМЯ` и `ВАШ_EMAIL`:

```bash
git filter-branch --force --env-filter '
if [ "$GIT_AUTHOR_EMAIL" = "project-author@example.invalid" ]; then
    GIT_AUTHOR_NAME="ВАШЕ ИМЯ"
    GIT_AUTHOR_EMAIL="ВАШ_EMAIL"
    export GIT_AUTHOR_NAME GIT_AUTHOR_EMAIL
fi
if [ "$GIT_COMMITTER_EMAIL" = "project-author@example.invalid" ]; then
    GIT_COMMITTER_NAME="ВАШЕ ИМЯ"
    GIT_COMMITTER_EMAIL="ВАШ_EMAIL"
    export GIT_COMMITTER_NAME GIT_COMMITTER_EMAIL
fi
' -- --all
```

Эту команду нужно выполнить один раз, до отправки проекта на GitHub. Она
меняет идентификаторы локальных коммитов, но сохраняет их сообщения и порядок.

Проверьте авторов и сообщения:

```bash
git log --format="%h %an <%ae> %s"
```

В сообщениях должны присутствовать пять основных этапов:

- `feat(config): ...` — этап 1, TOML-конфигурация;
- `feat(apk): ...` — этап 2, чтение APKINDEX;
- `feat(graph): ...` — этап 3, DFS без рекурсии;
- `feat(graph): ...` — этап 4, обратные зависимости;
- `feat(output): ...` — этап 5, PlantUML, SVG и ASCII.

Дополнительные коммиты по документации, Windows-сборке и оформлению графа
допустимы: все они также оформлены по Conventional Commits.

## 4. Проверить, что в репозиторий не попадут лишние файлы

Файл `.gitignore` уже подготовлен. Он исключает сборку, исполняемые файлы,
архивы, локальный `config_real.toml`, отчёты DOCX/PDF, кэши и настройки
редакторов.

Проверьте состояние:

```bash
git status --short
```

После распаковки и проверки в списке не должно быть папки `build`, файлов
`.exe`, `.dll`, архивов или отчёта Word. Папка сборки создаётся заново при
запуске `run_windows.sh` и не отправляется в GitHub.

Проверьте, что исходные файлы и обязательная структура отслеживаются Git:

```bash
git ls-files
```

Должны быть видны `README.md`, `.gitignore`, `src/`, `include/`, `tests/`,
`run.sh`, `run_windows.sh` и `CMakeLists.txt`.

## 5. Создать публичный репозиторий на сайте GitHub

1. Откройте <https://github.com/new> и войдите в свой аккаунт.
2. В поле Repository name укажите `variant23-dependency-visualizer`.
3. Выберите `Public`, чтобы преподаватель мог открыть репозиторий без запроса
   доступа.
4. Не добавляйте README, `.gitignore` и License: эти файлы уже есть в проекте.
5. Нажмите `Create repository`.
6. Скопируйте HTTPS-адрес созданного пустого репозитория. Он выглядит так:
   `https://github.com/ВАШ_ЛОГИН/variant23-dependency-visualizer.git`.

## 6. Отправить проект из MSYS2 UCRT64 на GitHub

Замените `ВАШ_ЛОГИН` на свой логин GitHub:

```bash
git branch -M main
git remote add origin https://github.com/ВАШ_ЛОГИН/variant23-dependency-visualizer.git
git push -u origin main
```

Если Git сообщает, что `origin` уже существует, выполните вместо `git remote add`:

```bash
git remote set-url origin https://github.com/ВАШ_ЛОГИН/variant23-dependency-visualizer.git
git push -u origin main
```

При запросе авторизации подтвердите вход через браузер или Git Credential
Manager. Обычный пароль от аккаунта GitHub не используется для HTTPS push.
Не вставляйте Personal Access Token в исходные файлы и не публикуйте его.

## 7. Проверить репозиторий на сайте

Откройте страницу репозитория на GitHub. Проверьте следующее:

- видимость репозитория — `Public`;
- в корне есть `README.md` и `.gitignore`;
- присутствуют каталоги `src/` и `tests/`;
- присутствует `include/` с общим заголовком;
- есть скрипты `run.sh` и `run_windows.sh`;
- есть `CMakeLists.txt`, `config.toml`, `test_repo.txt`, `examples/` и `docs/`;
- вкладка `Commits` показывает историю пяти этапов с сообщениями Conventional
  Commits;
- в репозитории нет `build/`, исполняемых файлов, архивов, DOCX/PDF и файлов
  настроек редактора;
- README открывается на главной странице и содержит описание, настройки,
  сборку, тесты и примеры использования.

Отчёт `Отчет_Вариант_23_черно_белый.docx` сдаётся преподавателю отдельно.
Его не нужно помещать в репозиторий исходного кода.

## 8. Команды для повторной проверки перед защитой

В каталоге проекта:

```bash
bash run_windows.sh
ctest --test-dir build --output-on-failure
./build/visualizer.exe examples/config_A.toml
./build/visualizer.exe examples/config_X.toml
./build/visualizer.exe examples/config_P.toml
./build/visualizer.exe examples/config_curl.toml
```

Последний пример требует подключения к Интернету и получает данные из
настоящего Alpine APKINDEX. После генерации `graph.svg` откройте его в браузере
или проводнике Windows.
