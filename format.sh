#!/bin/bash
grep -rl 'Copyright 2021' ./src | xargs sed -i 's/Copyright 2021/Copyright 2022/g'
grep -rl 'uni@vrsal.de' ./src | xargs sed -i 's/uni@vrsal.de/uni@vrsal.xyz/g'
grep -rl 'input-overlay' ./src | xargs sed -i 's/input-overlay/durchblick/g'
find ./src -iname *.h* -o -iname *.c* | xargs clang-format -style=File -i -verbose
