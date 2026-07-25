# iPhone Sticky Notes 📝

Стикеры-заметки для рабочего стола Windows в стиле iPhone/iOS. Основной вариант — Windhawk-мод, чтобы можно было добавить код из GitHub в Windhawk и установить на ПК. Также оставлена простая Python/Tkinter-версия как запасной вариант.

## Что есть в репозитории

- `mods/iphone-sticky-notes.wh.cpp` — готовый исходник Windhawk-мода.
- `sticky_notes.py` — отдельное Python-приложение без Windhawk.

## Возможности Windhawk-мода

- 📌 Плавающая кнопка **+** на экране для создания нового стикера.
- ✍️ В каждом стикере можно сразу печатать текст.
- 🖱️ Стикер можно перетаскивать за верхнюю панель.
- 🎨 Правый клик по стикеру или маленькая цветная кнопка вверху меняет цвет.
- ❌ Кнопка `x` закрывает конкретный стикер.
- 💾 Автосохранение текста, цвета и позиции в `%APPDATA%\WindhawkStickyNotes\notes.ini`.
- 📱 Яркие iOS-похожие цвета, полупрозрачность и скруглённые карточки.


## Авто-помощник установки в Windhawk для новичков

Полностью «тихой» установки собственного `.wh.cpp`-мода в Windhawk через официальный CLI сейчас нет, поэтому в репозитории есть безопасный авто-помощник. Он устанавливает Windhawk через `winget`, если Windhawk ещё не установлен, копирует код мода в буфер обмена и открывает Windhawk. После этого вам остаётся только вставить код и нажать compile/enable.

### Самый простой способ

1. Скачайте репозиторий с GitHub кнопкой **Code → Download ZIP**.
2. Распакуйте ZIP, например на рабочий стол.
3. Дважды нажмите `install-windhawk-sticky-notes.bat`.
4. Если Windows спросит разрешение PowerShell — разрешите запуск.
5. Скрипт откроет Windhawk и скопирует код мода в буфер обмена.
6. В Windhawk нажмите **Mods → Create a new mod**.
7. Удалите пример кода, нажмите **Ctrl+V**, потом **Compile Mod**.
8. Нажмите **Exit Editing Mode** и включите мод.

Файлы авто-помощника: `install-windhawk-sticky-notes.bat` и `scripts/install-windhawk-sticky-notes.ps1`.

## Как загрузить на GitHub и установить через Windhawk

> В этой среде нет настроенного GitHub remote, поэтому я подготовил код, сделал commit и PR-метаданные. Чтобы реально загрузить в ваш GitHub, добавьте remote и выполните `git push`.

1. Создайте пустой репозиторий на GitHub, например `iphone-sticky-notes`.
2. В терминале проекта выполните:

```bash
git remote add origin https://github.com/<ваш-логин>/iphone-sticky-notes.git
git push -u origin HEAD
```

3. Откройте файл на GitHub: `mods/iphone-sticky-notes.wh.cpp`.
4. Нажмите **Raw** и скопируйте raw-ссылку.
5. Откройте Windhawk → **Mods** → **Create a new mod** или **Install from URL**.
6. Вставьте raw-ссылку или скопируйте весь код из `mods/iphone-sticky-notes.wh.cpp`.
7. Нажмите compile/install, включите мод для `explorer.exe`.
8. Если кнопка **+** не появилась, перезапустите Explorer или выйдите/войдите в Windows.

## Ручная установка в Windhawk

1. Откройте Windhawk.
2. Создайте новый мод.
3. Скопируйте весь файл `mods/iphone-sticky-notes.wh.cpp`.
4. Скомпилируйте и включите мод.

## Python-версия без Windhawk

```bash
python sticky_notes.py
```

Дополнительные зависимости не нужны: используется стандартный Tkinter.
