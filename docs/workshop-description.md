# Steam Workshop description

Paste-ready text for the Workshop item. English first, Ukrainian below it, in one
description — the Workshop gives an item a single description field, and a reader
scrolls past the half that is not his far faster than he finds a second item.

Steam BBCode, not Markdown. Keep the `[url]` to the repository intact: the server
library is not on the Workshop and cannot be, so the description has to say where it is.

---

## English

```
[h1]OpenZone Radio[/h1]

DayZ ships with EIGHT radio frequencies. This mod turns that into as many as you
configure — up to thousands — so your server can have real channels instead of eight
crowded ones.

Any server, any map. Configured entirely from JSON.

[h1]⚠ Read this before installing[/h1]

[b]This mod alone will not widen the band.[/b] The eight frequencies are a limit inside
the game's own executable, not in its scripts, so no script mod can lift it.

A small [b]server-side native library[/b] does the lifting. It is NOT on the Workshop
and cannot be — it is a DLL, and the Workshop does not carry those. You build and
install it yourself from the repository:

[url=https://github.com/covalschi/openzone-radio]github.com/covalschi/openzone-radio[/url]

Without that library the mod still installs and runs. It detects the unpatched server,
says so in the log, and leaves the vanilla eight channels alone. Nothing breaks — you
simply get vanilla frequencies.

[b]Players need nothing.[/b] The frequency is resolved on the server, so a stock client
plays on a patched server normally. Only the server operator installs anything extra.

[h1]What you get[/h1]

[b]A measured band table.[/b] Everyone repeats that DayZ has seven frequencies. That
number is nowhere in the scripts — it lives in the engine. On start-up the mod measures
it: one radio spawned outside the world, indices walked until they stop being new, then
the radio is deleted. It measures rather than assumes.

[b]Radio profiles.[/b] Bands, steps and limits in JSON: who may tune where, in what
increments, between which bounds. Eleven ship as defaults.

[b]Push-to-talk with a squelch.[/b] A short burst of static on key down and key up,
heard from the radio itself at close range — not in your headphones.

[b]A frequency keypad.[/b] Type a frequency instead of pressing "next channel" until
you arrive.

[h1]Requirements[/h1]

[list]
[*] Community Framework
[*] For a wider band: the native library from the repository, on the server
[/list]

This mod depends on nothing else of ours. It was verified by booting a server with
Community Framework and this mod and nothing else.

[h1]Optional companions[/h1]

Two more mods live in the repository -- NOT on the Workshop, because their dependencies
are hard and would block the game for anyone without OpenZone Core and PDA. Download them
from the repository if you run the OpenZone PDA:

[list]
[*] [b]OpenZone Radio PDA[/b] — the radio as a board in a PDA device bay
[*] [b]OpenZone Radio VPP[/b] — an admin tab for editing profiles in game
[/list]

Both declare their dependencies hard, which in DayZ means a blocking window before the
game loads. Install a companion only together with what it needs.

[h1]Repacking: YES. Selling: NO.[/h1]

[b]You may repack this mod.[/b] Unpack it, change it, put it in a modpack, hand the
result to your players. Most DayZ mods forbid that, so saying nothing would read as a
refusal — this is not one.

Three conditions, and they are the licence rather than my preferences:

[list]
[*] Say where it came from, and link this page or the repository
[*] Keep the same licence on your repack (CC BY-NC-SA 4.0), and carry the NOTICE
[*] Do not sell it
[/list]

[b]Sign your repack with your OWN key, not mine.[/b] A key says who stands behind a
build, and only you can stand behind yours.

[b]Selling.[/b] Running this on a server that takes donations, sells cosmetics or sells
queue slots is fine — that is not commercial use. Selling the mod itself or any
derivative of it, or putting it behind a paywall (a "donate to download" one counts), is
not allowed.

[h1]Licence[/h1]

CC BY-NC-SA 4.0 plus the permissions above. Full text in the repository.
```

---

## Українська

```
[h1]OpenZone Radio[/h1]

У DayZ ВІСІМ радіочастот. Цей мод робить із них стільки, скільки ви налаштуєте — хоч
тисячі, — тож на сервері з'являються справжні канали замість восьми переповнених.

Будь-який сервер, будь-яка карта. Налаштовується цілком із JSON.

[h1]⚠ Прочитайте перед установкою[/h1]

[b]Сам по собі мод ефір не розширить.[/b] Вісім частот — межа в самому виконуваному
файлі гри, а не в її скриптах, тож жоден скриптовий мод її не підніме.

Піднімає невелика [b]серверна нативна бібліотека[/b]. У Workshop її НЕМАЄ і бути не
може — це DLL, а Workshop таких не носить. Ви збираєте й ставите її самі з репозиторію:

[url=https://github.com/covalschi/openzone-radio]github.com/covalschi/openzone-radio[/url]

Без цієї бібліотеки мод усе одно ставиться й працює. Він сам розпізнає непропатчений
сервер, каже про це в лог і лишає ванільні вісім каналів недоторканими. Нічого не
ламається — ви просто отримуєте ванільні частоти.

[b]Гравцям не потрібно нічого.[/b] Частоту вирішує сервер, тож звичайний клієнт грає на
пропатченому сервері як завжди. Щось додаткове ставить тільки власник сервера.

[h1]Що ви отримуєте[/h1]

[b]Зміряна таблиця частот.[/b] Усі повторюють, що в DayZ сім частот. Цього числа немає в
скриптах — воно живе в рушії. На старті мод міряє його сам: створює одну рацію поза
світом, проходить індекси, поки вони не перестануть бути новими, і видаляє рацію. Міряє,
а не припускає.

[b]Профілі рацій.[/b] Смуги, кроки й межі в JSON: кому куди можна налаштуватись, з яким
кроком і між якими межами. Одинадцять іде за умовчанням.

[b]Push-to-talk зі squelch.[/b] Короткий сплеск статики на натисканні й відпусканні,
чутний від самої рації зблизька — не в навушниках.

[b]Клавіатура частоти.[/b] Набрати частоту, а не тиснути «наступний канал», поки не
дійдеш.

[h1]Вимоги[/h1]

[list]
[*] Community Framework
[*] Для ширшого ефіру — нативна бібліотека з репозиторію, на сервері
[/list]

Від інших наших модів цей не залежить. Перевірено бутом сервера з Community Framework і
цим модом — і більше нічим.

[h1]Необов'язкові супутники[/h1]

Ще два моди живуть у репозиторії -- НЕ в Workshop, бо їхні залежності жорсткі й
заблокували б гру всім, у кого немає OpenZone Core і PDA. Беріть їх із репозиторію, якщо
крутите OpenZone PDA:

[list]
[*] [b]OpenZone Radio PDA[/b] — рація платою у відсіку приладу
[*] [b]OpenZone Radio VPP[/b] — адмінська вкладка для правки профілів у грі
[/list]

Обидва оголошують свої залежності жорстко, а це в DayZ означає блокуюче вікно ще до
завантаження гри. Ставте супутника лише разом із тим, що йому потрібно.

[h1]Перепаковувати: ТАК. Продавати: НІ.[/h1]

[b]Цей мод можна перепаковувати.[/b] Розпакуйте, змініть, покладіть у модпак, віддайте
результат своїм гравцям. Більшість модів DayZ це забороняє, тож мовчання читалося б як
відмова — це не відмова.

Три умови, і це ліцензія, а не мої побажання:

[list]
[*] Скажіть, звідки це, і дайте посилання на цю сторінку або репозиторій
[*] Збережіть ту саму ліцензію на перепаку (CC BY-NC-SA 4.0) і несіть NOTICE
[*] Не продавайте
[/list]

[b]Підписуйте перепак СВОЇМ ключем, не моїм.[/b] Ключ каже, хто стоїть за збіркою, а за
свою можете ручатися тільки ви.

[b]Продаж.[/b] Крутити це на сервері, який приймає донати, продає косметику чи місця в
черзі, — нормально, це не комерційне використання. А продавати сам мод чи будь-яку його
похідну, або давати доступ за оплату («задонать, щоб завантажити» теж рахується), — не
можна.

[h1]Ліцензія[/h1]

CC BY-NC-SA 4.0 плюс дозволи вище. Повний текст — у репозиторії.
```
