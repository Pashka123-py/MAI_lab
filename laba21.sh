#!/bin/bash

file_size=40


for file in /root/programms_c/test_1/*; do
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
                # echo "$new_name"

            fi
            path_old=$(dirname "$file")
            new_path="${path_old}/${new_name}"
            echo "$new_path"
            mv "$file" "$new_path"
            
        fi
    fi
done