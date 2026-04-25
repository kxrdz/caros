#!/bin/sh

for file in ./4x3/*.svg
do 
	echo "Convert $file ..."
	filename=$(basename -- "$file")
	extension="${filename##*.}"
	filename="${filename%.*}"
	echo "$file"

	magick "$file" -resize 40x30 "./4x3png/$filename.png";
done

