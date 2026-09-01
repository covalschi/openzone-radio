# Publishing to the Steam Workshop

What is ready, what is not, and what has to be decided by a person. Written
2026-09-01 while preparing the first publication.

*Українською — нижче, у другій половині файла.*

## Ready

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

**Signing keys.** The build says `no signing keys: keys/ does not exist, so the pbo is
unsigned`. Servers running `verifySignatures = 2` will refuse an unsigned pbo. Making a
key is one DSUtils command, but the private key is a **secret that must never enter the
repository**, and where it lives is the owner's decision. Decide before the first
publish: re-signing later changes the key servers already trust.

**A picture and a logo.** `mod.cpp` deliberately does not point at `picture` or `logo`
files: DayZ draws a blank for a missing texture and says nothing, so a wrong path is
worse than none. Add the fields when the art exists.

**Three Workshop items or one.** The repository builds three mods with different
dependencies. Publishing all three as one item would give every subscriber the two glue
mods, whose dependencies are **hard** — a blocking window before the game loads for
anyone without OpenZone Core and PDA. Three items is the safe shape; one item is
defensible only if the glue is dropped from it.

**The version number.** `mod.cpp` says `0.1` in all three. It is a claim about maturity
that the Workshop shows to strangers.

**The unproven claim.** The README says so plainly and the Workshop text does not repeat
it: nobody has yet *heard* two players on distant channels fail to hear each other. The
frequencies are distinct and the router keys on their bytes, so the risk is small — but
"measured distinct" and "heard separated" are different sentences. Two players, two
handheld radios, eight presses of tune apart.

## The publish itself

The DayZ Tools **Publisher** uploads a `@Mod` folder and writes `meta.cpp` itself; do
not write that file by hand. The owner drives Publisher — the GUI tools are not run from
here.

---

# Публікація в Steam Workshop

Що готове, що ні, і що мусить вирішити людина. Написано 2026-09-01 під час підготовки до
першої публікації.

## Готове

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

**Ключі підпису.** Збірка каже `no signing keys: keys/ does not exist, so the pbo is
unsigned`. Сервери з `verifySignatures = 2` непідписаний pbo відкинуть. Зробити ключ —
одна команда DSUtils, але приватний ключ — **таємниця, якій не місце в репозиторії**, і
де він житиме, вирішує власник. Вирішити треба до першої публікації: перепідписати
пізніше означає змінити ключ, якому сервери вже довіряють.

**Картинка й логотип.** `mod.cpp` навмисне не вказує на `picture` і `logo`: DayZ малює
порожнечу замість відсутньої текстури й мовчить, тож хибний шлях гірший за відсутній.
Додати поля, коли з'явиться арт.

**Три елементи Workshop чи один.** Репозиторій збирає три моди з різними залежностями.
Опублікувати всі три одним елементом означає дати кожному підписнику дві склейки, чиї
залежності **жорсткі**, — блокуюче вікно ще до завантаження гри для всіх, у кого немає
OpenZone Core і PDA. Три елементи — безпечна форма; один виправданий лише якщо склейки з
нього прибрати.

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
