#!/bin/bash

# Функция для отображения справки
help() {
    echo "Использование: $0 [ОПЦИИ]"
    echo "Скрипт для переименования файлов в папке test3 на основе их размера"
    echo ""
    echo "Описание работы:"
    echo "  Скрипт проходит по всем файлам в папке test3 и对于那些, чей размер меньше"
    echo "  заданного значения (по умолчанию 100 байт), выполняет переименование:"
    echo "  1. Если длина имени файла (без расширения) меньше длины расширения,"
    echo "     то новое имя формируется как 'имя.имя'"
    echo "  2. Если длина имени больше или равна длине расширения,"
    echo "     то новое имя формируется как 'имя.первые_буквы_имени'"
    echo "     где количество первых букв равно длине расширения"
    echo ""
    echo "Параметры:"
    echo "  -h, --help    показать эту справку и выйти"
    echo ""
    echo "Переменные для настройки:"
    echo "  file_size     размер файла для отбора (по умолчанию: 100 байт)"
    echo "  TARGET_DIR    целевая директория (по умолчанию: test3 в папке скрипта)"
    echo ""
    echo "Пример запуска:"
    echo "  $0           # запуск скрипта с параметрами по умолчанию"
    echo "  $0 --help    # показать справку"
}

# Обработка аргументов командной строки
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    help
    exit 0
fi

# Определяем директорию, в которой находится сам скрипт
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Формируем путь к искомой папке test (название папки может быть любое, в данном случае программа выполняется над файлами папки test)
TARGET_DIR="$SCRIPT_DIR/test"

# В этой переменной можем задать произвольный размер для отбора файлов
file_size=40  


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
