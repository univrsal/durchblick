#!/bin/bash
last_year=$(date -d "last year" +%Y)
current_year=$(date +%Y)
grep -rl 'Copyright ${last_year}' ./src | xargs sed -i 's/Copyright ${last_year}/Copyright ${current_year}/g'
#grep -rl 'uni@vrsal.de' ./src | xargs sed -i 's/uni@vrsal.de/uni@vrsal.xyz/g'
find ./src -iname *.h* -o -iname *.c* | xargs clang-format -style=File -i -verbose
