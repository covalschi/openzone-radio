# Publishing to the Steam Workshop

**Published 2026-09-01:** https://steamcommunity.com/sharedfiles/filedetails/?id=3794105144

Only `@OpenZone_Radio` is on the Workshop. What is ready, what is not, and what has to
be decided by a person.

*Українською — нижче, у другій половині файла.*

## Ready

- **Only `@OpenZone_Radio` goes to the Workshop.** Owner's decision 2026-09-01. The two
  glue mods ship through this repository and nowhere else: their dependencies are hard,
  so a subscriber without OpenZone Core and PDA would meet a blocking window before the
  game loads. One Workshop item, three mods in git.
- **Signed.** Key `ZoneProtocol`, created 2026-09-01 with `DSCreateKey`. All three pbos
  carry `*.ZoneProtocol.bisign`, and `keys/ZoneProtocol.bikey` ships inside every `@Mod`
  folder and at the repository root, so a server running `verifySignatures = 2` can
  accept them.
  > **The private key is not in git and must never be.** `keys/ZoneProtocol.biprivatekey`
  > exists only on the build machine and is covered by `.gitignore`. **Back it up
  > somewhere outside this repository.** Losing it means every future release is signed
  > by a different key, and every server that installed the old `.bikey` has to replace
  > it. Anyone who obtains it can sign a mod as you.
- **`@OpenZone_Radio` stands alone.** Verified, not assumed: a server booted with
  Community Framework and this mod and nothing else — no Core, no PDA, no VPP, no glue —
  and reported `radio loaded: bands=8 profiles=11` with zero errors. `requiredAddons`
  names only `DZ_Data`, `DZ_Scripts`, `JM_CF_Scripts`, and every script symbol in the
  pbo lives in its own `OZR_` namespace.
- **`mod.cpp` in all three mods.** Without it DayZ shows the bare folder name and
  nothing else — no author, no version, no description. Each one names its hard
  dependencies in the description, so a server owner learns about them before the
  blocking window rather than from it.
- **The band works without the native library.** The mod detects the unpatched engine,
  writes `the engine's frequency table is not an even grid (8 bands) — radio profiles
  stay unapplied; this is what an unpatched server looks like`, and leaves vanilla
  alone. Nothing breaks; the operator simply gets eight channels.
- **Workshop description** in English and Ukrainian, Steam BBCode, ready to paste:
  [`workshop-description.md`](workshop-description.md). It warns about the native
  library and links the repository, because the Workshop cannot carry a DLL.

## Not ready, and each needs a person

**A picture and a logo.** `mod.cpp` deliberately does not point at `picture` or `logo`
files: DayZ draws a blank for a missing texture and says nothing, so a wrong path is
worse than none. Add the fields when the art exists.

**The version number.** `mod.cpp` says `0.1` in all three. It is a claim about maturity
that the Workshop shows to strangers.

**The unproven claim.** The README says so plainly and the Workshop text does not repeat
it: nobody has yet *heard* two players on distant channels fail to hear each other. The
frequencies are distinct and the router keys on their bytes, so the risk is small — but
"measured distinct" and "heard separated" are different sentences. Two players, two
handheld radios, eight presses of tune apart.

## Publishing again

Give **Publisher** the folder itself:

```
E:\openzone\openzone-radio\@OpenZone_Radio
```

Not the repository, not `addons` inside it.

`meta.cpp` in that folder is what ties it to Workshop item 3794105144: without it the
next upload creates a SECOND item instead of updating this one. **Keep it, and keep it
in git.**

> The first publish (2026-09-01) left none — the folder had `addons`, `keys` and
> `mod.cpp` and nothing else, and a search of the machine for a `meta.cpp` or for the
> item id turned up nothing at all. It was written by hand afterwards, from the id in
> the Workshop URL, in the format an installed Workshop mod uses:
>
> ```
> protocol = 1;
> publishedid = 3794105144;
> name = "OpenZone Radio";
> ```
>
> `timestamp` is omitted deliberately: a real one carries a value Publisher generates,
> and inventing a number to sit in a field that means "when this build was uploaded"
> would be a lie in a file whose whole job is bookkeeping. Publisher fills it on the
> next upload.
>
> If a later publish still creates a second item, the id here is not being read, and
> the answer is to publish once more from a folder where Publisher itself wrote the
> file — then commit that version over this one.

`mod_build` overwrites `addons/` and leaves `mod.cpp`, `keys/` and `meta.cpp` alone, so
re-publishing from the same folder is safe.

The owner drives Publisher — the GUI tools are not run from here.

---

# Публікація в Steam Workshop

Що готове, що ні, і що мусить вирішити людина. Написано 2026-09-01 під час підготовки до
першої публікації.

## Готове

- **У Workshop іде ЛИШЕ `@OpenZone_Radio`.** Рішення власника 2026-09-01. Дві склейки
  розповсюджуються через цей репозиторій і більше ніяк: їхні залежності жорсткі, тож
  підписник без OpenZone Core і PDA дістав би блокуюче вікно ще до завантаження гри.
  Один елемент Workshop, три моди в git.
- **Підписано.** Ключ `ZoneProtocol`, створений 2026-09-01 через `DSCreateKey`. Усі три
  pbo несуть `*.ZoneProtocol.bisign`, а `keys/ZoneProtocol.bikey` лежить і в кожній теці
  `@Mod`, і в корені репозиторію, тож сервер із `verifySignatures = 2` їх прийме.
  > **Приватного ключа в git немає, і бути не мусить.** `keys/ZoneProtocol.biprivatekey`
  > існує лише на складальній машині й покритий `.gitignore`. **Зробіть резервну копію
  > поза цим репозиторієм.** Втратити його означає, що кожен наступний випуск підписаний
  > іншим ключем, і кожен сервер, який поставив старий `.bikey`, мусить його замінити.
  > Хто його дістане, зможе підписувати моди вашим ім'ям.
- **`@OpenZone_Radio` самостійний.** Перевірено, а не припущено: сервер піднято з
  Community Framework і цим модом — без ядра, КПК, VPP і склейок — і він відзвітував
  `radio loaded: bands=8 profiles=11` без жодної помилки. У `requiredAddons` лише
  `DZ_Data`, `DZ_Scripts`, `JM_CF_Scripts`, а всі скриптові імена pbo живуть у власному
  просторі `OZR_`.
- **`mod.cpp` в усіх трьох модах.** Без нього DayZ показує голу назву теки й більше
  нічого — ні автора, ні версії, ні опису. Кожен називає свої жорсткі залежності в
  описі, щоб власник сервера дізнався про них до блокуючого вікна, а не з нього.
- **Ефір працює й без нативної бібліотеки.** Мод розпізнає непропатчений рушій, пише
  `the engine's frequency table is not an even grid (8 bands) — radio profiles stay
  unapplied; this is what an unpatched server looks like` і лишає ваніль недоторканою.
  Нічого не ламається; оператор просто отримує вісім каналів.
- **Опис для Workshop** англійською та українською, у Steam BBCode, готовий до вставки:
  [`workshop-description.md`](workshop-description.md). Він попереджає про нативну
  бібліотеку й веде в репозиторій, бо DLL Workshop нести не може.

## Не готове, і кожне потребує людини

**Картинка й логотип.** `mod.cpp` навмисне не вказує на `picture` і `logo`: DayZ малює
порожнечу замість відсутньої текстури й мовчить, тож хибний шлях гірший за відсутній.
Додати поля, коли з'явиться арт.

**Номер версії.** У `mod.cpp` в усіх трьох стоїть `0.1`. Це твердження про зрілість, яке
Workshop показує стороннім.

**Недоведене твердження.** README каже це прямо, а текст Workshop не повторює: ніхто ще
не **чув**, як двоє гравців на далеких каналах не чують одне одного. Частоти різні, а
маршрутизатор ключується на їхніх байтах, тож ризик малий — але «зміряно різними» і
«почуто роздільними» це різні речення. Двоє гравців, дві ручні рації, вісім натискань
«налаштувати» одне від одного.

## Сама публікація

**Publisher** із DayZ Tools вивантажує теку `@Mod` і пише `meta.cpp` сам; руками цей
файл не пишуть. Publisher запускає власник — GUI-інструменти звідси не запускаються.
