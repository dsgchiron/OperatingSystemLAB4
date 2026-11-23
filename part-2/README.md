# Частина 2. Права доступу

Тема: користувачі, групи, права доступу. Інформація: OSTEP [39. Interlude: Files and Directories](https://pages.cs.wisc.edu/~remzi/OSTEP/file-intro.pdf) (розділ Permission Bits And Access Control Lists).

## Мета

Перевірити на практиці, за якими правилами ядро вирішує, чи може процес прочитати або змінити файл і створити або видалити ім'я в каталозі.

## Дано

`setup-users.sh` створив:

| User    | Groups          |
| ------- | --------------- |
| `alice` | `alice`, `team` |
| `bob`   | `bob`, `team`   |
| `carol` | `carol`         |

```text
drwxrwxr-x alice team  /srv/lab4
-rw-r----- alice team  /srv/lab4/plan.txt    (вміст: secret)
-rw-r--r-- alice alice /srv/lab4/notes.txt   (вміст: hello)
```

`sudo -u bob COMMAND` запускає команду як процес з UID і групами `bob`. `tee -a FILE` друкує на екран те, що отримує на вхід, і дописує це в кінець FILE — тому текст з'являється на екрані навіть тоді, коли дописати не вдалося.

## Завдання

Виконайте команди по порядку (команди з `chmod` змінюють права для наступних). Для кожної запишіть у таблицю в `answers.md` результат (вдалося чи яка помилка) і коротко поясніть, чому: чий набір бітів перевірявся (власника, групи чи інших) і на чому — на файлі чи на каталозі; для `chmod` — хто має право змінювати права.

Приклад для `sudo -u carol cat /srv/lab4/notes.txt`: результат — вдалося, `hello`; чому — `carol` не власниця `notes.txt` і не в групі `alice`, тому перевіряються біти інших на файлі: `r--`, читання дозволено.

```sh
sudo -u bob cat /srv/lab4/plan.txt                   # 1
echo more | sudo -u bob tee -a /srv/lab4/plan.txt    # 2
sudo -u carol cat /srv/lab4/plan.txt                 # 3
sudo -u alice chmod 604 /srv/lab4/plan.txt           # 4
sudo -u bob cat /srv/lab4/plan.txt                   # 5
sudo -u carol cat /srv/lab4/plan.txt                 # 6
sudo -u alice chmod 044 /srv/lab4/plan.txt           # 7
sudo -u alice cat /srv/lab4/plan.txt                 # 8
sudo -u bob chmod 666 /srv/lab4/plan.txt             # 9
sudo -u bob rm -f /srv/lab4/notes.txt                # 10
sudo -u carol touch /srv/lab4/carol.txt              # 11
```
