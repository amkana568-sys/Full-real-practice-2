# Публикация проекта в GitHub

Инструкция предназначена для папки проекта, в которой уже есть каталог `.git`.
Повторно выполнять `git init` не нужно: история пяти этапов уже создана.

## 1. Открыть проект в MSYS2 UCRT64

Откройте MSYS2 UCRT64 и перейдите в каталог проекта. Для компьютера, где
проект расположен по пути из примера, команда такая:

```bash
cd "/c/Users/STRIX/Desktop/учеба/конфУправление/пр3/proga"
```

Проверьте Git:

```bash
git --version
```

Если команда не найдена, установите Git из MSYS2:

```bash
pacman -S --needed git
```

## 2. Настроить имя автора коммитов

Укажите свои имя и email. Email должен быть привязан к GitHub или заменён
на адрес noreply из настроек GitHub.

```bash
git config user.name "Имя Фамилия"
git config user.email "ВАШ_EMAIL"
```

Проверьте локальную историю:

```bash
git log --oneline
```

В истории должны быть сообщения `feat(config)`, `feat(apk)`, два коммита
`feat(graph)` и `feat(output)`. Это пять этапов задания.

## 3. Не добавлять отчёт Word и файлы сборки

Отчёт DOCX сдаётся отдельно. В соответствии с критериями репозиторий
исходного кода не должен содержать бинарные файлы и артефакты сборки.

Новый `.gitignore` исключает папку `build`, исполняемые файлы, архивы,
локальную конфигурацию и DOCX-отчёт. Если DOCX уже отслеживается Git,
уберите его только из индекса, сохранив файл на компьютере:

```bash
git rm --cached --ignore-unmatch -- "Отчет_Вариант_23.docx"
```

Посмотрите, что будет добавлено в коммит:

```bash
git status --short
```

В списке не должно быть файлов из `build/`, `.exe`, `.a`, архивов или DOCX.

## 4. Сохранить последние изменения

Добавьте актуальные файлы проекта и документацию:

```bash
git add -A
git status --short
git commit -m "chore(repo): prepare project for public review"
```

Команда `git add -A` безопасна здесь потому, что `.gitignore` уже исключает
сборку, бинарные файлы, архивы и отдельный Word-отчёт.

## 5. Исправить автора старых учебных коммитов (рекомендуется)

В исходной локальной истории учебные коммиты были созданы с техническим
автором `Project Author <project-author@example.invalid>`. Чтобы в GitHub
отображалось ваше имя, до первой публикации можно заменить автора старых
коммитов. Замените `ВАШЕ ИМЯ` и `ВАШ_EMAIL` в команде на свои данные:

```bash
git filter-branch --force \
  --env-filter '
OLD_EMAIL="project-author@example.invalid"
if [ "$GIT_AUTHOR_EMAIL" = "$OLD_EMAIL" ]; then
    GIT_AUTHOR_NAME="ВАШЕ ИМЯ"
    GIT_AUTHOR_EMAIL="ВАШ_EMAIL"
    export GIT_AUTHOR_NAME GIT_AUTHOR_EMAIL
fi
if [ "$GIT_COMMITTER_EMAIL" = "$OLD_EMAIL" ]; then
    GIT_COMMITTER_NAME="ВАШЕ ИМЯ"
    GIT_COMMITTER_EMAIL="ВАШ_EMAIL"
    export GIT_COMMITTER_NAME GIT_COMMITTER_EMAIL
fi
' \
  --index-filter 'git rm --cached --ignore-unmatch -- "Отчет_Вариант_23.docx"' \
  --prune-empty -- --all
```

Эта операция переписывает локальные идентификаторы коммитов. Выполняйте её
только до первой отправки проекта в GitHub. Сообщения пяти этапов сохраняются.
Файл DOCX останется в папке проекта, но будет удалён из всей публикуемой
истории Git.

Проверьте результат:

```bash
git log --format="%h %an <%ae> %s"
git ls-tree -r --name-only HEAD
```

В списке файлов не должно быть `Отчет_Вариант_23.docx`, `build/`, архивов
и исполняемых файлов.

## 6. Создать пустой репозиторий на GitHub

1. Откройте <https://github.com/new> и войдите в свой аккаунт.
2. Введите имя, например `variant23-dependency-visualizer`.
3. Выберите **Public**, чтобы преподаватель мог открыть проект по ссылке.
4. Не ставьте флажки добавления README, `.gitignore` и License: эти файлы
   уже есть в локальном проекте.
5. Нажмите **Create repository** и скопируйте HTTPS URL репозитория.

## 7. Отправить локальную историю в GitHub

Замените `USERNAME` на свой логин GitHub:

```bash
git branch -M main
git remote add origin https://github.com/USERNAME/variant23-dependency-visualizer.git
git push -u origin main
```

Если Git сообщает, что `origin` уже существует, вместо `git remote add`
выполните:

```bash
git remote set-url origin https://github.com/USERNAME/variant23-dependency-visualizer.git
git push -u origin main
```

При запросе авторизации войдите через браузер или используйте Git Credential
Manager / Personal Access Token. Пароль от аккаунта GitHub в качестве пароля
Git по HTTPS не используется. Не записывайте токен в файлы проекта.

## 8. Проверить репозиторий на сайте

Откройте созданную страницу GitHub. Проверьте, что в корне видны:

- `README.md` и `.gitignore`;
- `src/` и `include/`;
- `tests/`;
- `run.sh` и `run_windows.sh`;
- `CMakeLists.txt`, `config.toml`, `test_repo.txt`;
- `examples/` и `docs/`.

На вкладке **Commits** должны быть видны пять этапов с сообщениями
Conventional Commits. На вкладке **Code** не должно быть папки `build`,
архивов, исполняемых файлов и Word-отчёта.

Word-отчёт `Отчет_Вариант_23.docx` храните отдельно и сдавайте преподавателю
отдельным файлом.
