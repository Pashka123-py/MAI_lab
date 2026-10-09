#!/bin/bash

# Функция для отображения справки
help() {
    echo "Использование: $0 [ОПЦИИ]"
    echo "Скрипт для переименования файлов в папке test1 на основе их размера"
    echo ""
    echo "Описание работы:"
    echo "  Скрипт проходит по всем файлам в папке test1 и проверяет, чей размер меньше"
    echo "  заданного значения (по умолчанию 100 байт), выполняет переименование:"
    echo "  1. Если длина имени файла (без расширения) меньше длины расширения,"
    echo "     то новое имя формируется как 'имя.имя'"
    echo "  2. Если длина имени больше или равна длине расширения,"
    echo "     то новое имя формируется как 'имя.первые_буквы_имени'"
    echo "     где количество первых букв равно длине расширения"
    echo ""
    echo "Параметры:"
    echo "  -h, --help    показать эту справку и выйти"
    echo "  <имя_папки>   имя папки (относительно расположения скрипта),"
    echo "                файлы в которой будут обработаны"
    echo ""
}

# Обработка аргументов командной строки
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    help
    exit 0
fi

# Проверка, передан ли параметр с именем папки
if [ -z "$1" ]; then
    echo "Ошибка: укажите директорию для обработки"
    echo "Для справки: $0 --help"
    exit 1
fi

# Определяем директорию, в которой находится сам скрипт
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Формируем путь к искомой папке test (название папки может быть любое, в данном случае программа выполняется над файлами папки test3)
TARGET_DIR="$SCRIPT_DIR/$1"

if [ ! -d "$TARGET_DIR" ]; then
    echo "Ошибка: Директория '$TARGET_DIR' не существует!"
    exit 1
fi

# В этой переменной можем задать произвольный размер для отбора файлов
file_size=100  


for file in "$TARGET_DIR"/*; do

    if [ -f "$file" ]; then

        size=$(stat -c%s "$file")
        name=$(basename "$file")
        
        if [ $size -lt $file_size ]; then

            suff="${name##*.}"
            size_suff=${#suff}
            name_without_suff="${name%.*}"
            size_name_without_suff=${#name_without_suff}
            
            if [ $size_name_without_suff -lt $size_suff ]; then
                new_name="${name_without_suff}.${name_without_suff}"
                
            else
            
                name_first_letter="${name:0:$size_suff}"
                new_name="${name_without_suff}.${name_first_letter}"

            fi
            
            path_old=$(dirname "$file")
            new_path="${path_old}/${new_name}"
            echo "$name -> $new_name"
            mv "$file" "$new_path"  
            
        fi
    fi
done
