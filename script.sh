#!/bin/bash

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 <character>"
  exit 1
fi

CHARACTER="$1"
count=0
regex="^[[:upper:]][[:alnum:][:space:],.!?]*[.!?]$"
while IFS= read -r line; do
  if [[ $line =~ $regex && ! $line =~ ,\ *și ]]; then
    if [[ $line =~ $CHARACTER ]]; then
      ((count++))
    fi
  fi
done

echo "$count"

