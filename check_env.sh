#!/bin/sh
# Перевіряє, що середовище придатне для ЛР4. Запуск: sh check_env.sh
status=0
need() {
    if command -v "$1" >/dev/null 2>&1; then
        echo "ok      $1"
    else
        echo "MISSING $1  ->  sudo apt install $2"
        status=1
    fi
}
need gcc build-essential
need make build-essential
need strace strace
need sudo sudo

fs=$(stat -f -c %T .)
case "$fs" in
ext2/ext3 | xfs)
    echo "ok      файлова система поточного каталогу: $fs"
    ;;
*)
    echo "FAIL    файлова система поточного каталогу: $fs. Скопіюйте lab4 у домашній каталог VM: cp -r \"$PWD\" ~/lab4 && cd ~/lab4"
    status=1
    ;;
esac

if id alice >/dev/null 2>&1 && id bob >/dev/null 2>&1 && id carol >/dev/null 2>&1 &&
    [ -d /srv/lab4 ]; then
    echo "ok      користувачі alice, bob, carol і /srv/lab4"
else
    echo "FAIL    немає підготовлених користувачів: sudo sh setup-users.sh"
    status=1
fi

tmp=$(mktemp -d ./check.XXXXXX) || exit 1
if echo x > "$tmp/a" && ln "$tmp/a" "$tmp/b" 2>/dev/null &&
    [ "$(stat -c %h "$tmp/a")" -eq 2 ]; then
    echo "ok      жорсткі посилання"
else
    echo "FAIL    жорсткі посилання в поточному каталозі не працюють"
    status=1
fi
rm -rf "$tmp"

echo "kernel: $(uname -r), arch: $(uname -m), gcc: $(gcc -dumpversion)"
[ $status -eq 0 ] && echo "Усе готово." || echo "Є проблеми, див. вище."
exit $status
