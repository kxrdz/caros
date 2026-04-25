#!/bin/sh

for file in ./*.png
do 
	echo "Resizing file $file ..."
	#convert -resize 256x256 "$file" "$file" 
	convert -resize 70% "$file" "$file" 
done

