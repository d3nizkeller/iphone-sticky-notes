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
