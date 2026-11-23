# Частина 3. Надійна заміна файлу

Тема: запис у файл, page cache, `fsync`, `rename`. Інформація: OSTEP [39. Interlude: Files and Directories](https://pages.cs.wisc.edu/~remzi/OSTEP/file-intro.pdf) (розділи Writing Immediately With fsync() і Renaming Files).

## Мета

Написати заміну вмісту файлу, після якої за будь-якого переривання на місці файлу лежить або повністю стара, або повністю нова версія, і зрозуміти, що саме гарантує кожен крок.

## Дано

`save.c`: програма читає новий вміст зі stdin і записує його у FILE одним із двох способів:

```sh
./save naive|safe FILE [CHECKPOINT] < NEW_CONTENT
```

- `naive` (готовий; запрацює, щойно ви заповните TODO 1): відкрити FILE з `O_TRUNC`, записати, закрити.
- `safe` (потрібно доповнити): записати все в тимчасовий файл `FILE.tmp` у тому самому каталозі, `fsync` файлу, `rename` поверх FILE, `fsync` каталогу.
- CHECKPOINT — назва контрольної точки: `truncated` у `naive`; `created`, `written`, `synced`, `renamed`, `dir-synced` у `safe` (їх видно в `save.c`). Дійшовши до неї, програма вбиває сама себе сигналом SIGKILL: процес зникає миттєво, як при аварійному завершенні. Shell надрукує `Killed`, код виходу — 137 (128 + 9, номер SIGKILL).

## Завдання

Усі команди виконуйте в каталозі `part3-replace`; програма збирається командою `make -C ..`.

1. Заповніть **TODO 1** (`write_all`): записати всі `n` байтів. Перевірте наївний спосіб:

   ```sh
   cd ~/lab4/part3-replace
   make -C ..
   echo v1 > cfg
   echo v2 | ./save naive cfg
   cat cfg
   echo v1 > cfg
   echo v2 | ./save naive cfg truncated
   echo $?
   cat cfg
   ```

   Що лежить у `cfg` після переривання на `truncated`?

2. Заповніть **TODO 2–6** у `replace_safe`. При будь-якій помилці програма має надрукувати її і завершитися з кодом 1 (`echo $?`). Якщо помилка сталася до заміни, FILE має лишитися як був, без `FILE.tmp` поруч.

   Перевірте результат і послідовність системних викликів:

   ```sh
   make -C ..
   echo v1 > cfg
   ls -i cfg
   echo v2 | strace -e trace=openat,write,fsync,close,/rename ./save safe cfg
   cat cfg
   ls -i cfg
   ```

   Перші рядки strace (`openat` файлів `.so` і `ld.so.cache`, `close(3)`) — завантаження бібліотек, а `write(2, "saved ...` наприкінці — повідомлення самої програми, перемішане з виводом strace; до `replace_safe` вони не належать. `/rename` у `-e trace=` означає всі виклики, у назві яких є `rename`. Чи збігається послідовність викликів із кроками з «Дано»? Чи змінився номер inode у `cfg` і чому?

3. **Що лишається після збою.** Перевірте, що опиниться на місці `cfg`, якщо `safe` аварійно завершиться посеред заміни `v1` на `v2`. Контрольна точка задає момент, у який програма вбиває сама себе:

   | Точка        | Що програма встигла зробити         |
   | ------------ | ----------------------------------- |
   | `created`    | створити `cfg.tmp`                  |
   | `written`    | записати в нього дані               |
   | `synced`     | `fsync` і закрити `cfg.tmp`         |
   | `renamed`    | `rename`                            |
   | `dir-synced` | `fsync` каталогу, тобто все         |

   Для точки `created`:

   ```sh
   echo v1 > cfg
   echo v2 | ./save safe cfg created
   cat cfg
   cat cfg.tmp
   ```

   Якщо `cat cfg.tmp` нічого не надрукував, файл є, але порожній; якщо `No such file or directory` — файлу немає. Повторіть ці чотири команди для решти точок, замінюючи `created`, і запишіть у таблицю в `answers.md`, що лежить у `cfg` і в `cfg.tmp`.

   Потім заповніть останню колонку таблиці. Уявіть, що в той самий момент замість SIGKILL вимкнули живлення комп'ютера: яка версія `cfg` (`v1`, `v2`, порожній чи недописаний) могла б опинитися на диску після перезавантаження? Вважайте, що `v1` уже був на диску до запуску, а файлова система — ext4 з налаштуваннями за замовчуванням.

4. **Помилка запису.** Змонтуйте маленьку файлову систему в пам'яті (tmpfs на 1 МіБ) і спробуйте записати в неї файл з 2 МіБ нулів (`big.bin`):

   ```sh
   sudo mkdir -p /mnt/small
   sudo mount -t tmpfs -o size=1m,mode=1777 tmpfs /mnt/small
   echo old > /mnt/small/cfg
   head -c 2M /dev/zero > big.bin
   ./save safe /mnt/small/cfg < big.bin
   echo $?
   ls -l /mnt/small
   cat /mnt/small/cfg
   ./save naive /mnt/small/cfg < big.bin
   echo $?
   ls -l /mnt/small
   sudo umount /mnt/small
   ```

   Ваш `safe` має надрукувати помилку, повернути 1, залишити `cfg` зі старим вмістом і не залишити `cfg.tmp`. Якщо це не так — виправте код. Що сталося з `cfg` у наївному способі?
