#!/bin/sh
# Створює користувачів і файли для частини 2. Запуск: sudo sh setup-users.sh
# Можна запускати повторно: каталог /srv/lab4 буде створено заново.
set -e
if [ "$(id -u)" -ne 0 ]; then
    echo "Запустіть через sudo: sudo sh setup-users.sh"
    exit 1
fi

getent group team >/dev/null || groupadd team
for user in alice bob carol; do
    id "$user" >/dev/null 2>&1 || useradd -m -s /bin/bash "$user"
done
usermod -aG team alice
usermod -aG team bob
gpasswd -d carol team >/dev/null 2>&1 || true

rm -rf /srv/lab4
mkdir -p /srv/lab4
chown alice:team /srv/lab4
chmod 775 /srv/lab4

echo secret > /srv/lab4/plan.txt
chown alice:team /srv/lab4/plan.txt
chmod 640 /srv/lab4/plan.txt

echo hello > /srv/lab4/notes.txt
chown alice:alice /srv/lab4/notes.txt
chmod 644 /srv/lab4/notes.txt

id alice
id bob
id carol
ls -la /srv/lab4
