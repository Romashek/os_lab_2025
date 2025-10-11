#!/bin/bash

count=$#

if [ $count -eq 0 ]; then
    echo "Нет аргументов!"
    exit 1
fi

sum=0
for num in "$@"; do
    sum=$((sum + num))
done

avg=$((sum / count))

echo "Количество аргументов: $count"
echo "Среднее арифметическое: $avg"

