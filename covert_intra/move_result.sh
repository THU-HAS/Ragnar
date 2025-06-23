#!/bin/bash

dir=$1
target="$1/$2"

if [ ! -d "$target" ]; then
  mkdir "$target"
fi

for file in $1/result*; do
  if [ -f "$file" ]; then
    mv "$file" "$target/"
  fi
done

