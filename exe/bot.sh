#!/bin/bash
LIMIT=$1
CONNECTADDR=$2
SAVEJ=$3

if [ "$SAVEJ" = "TRUE" ]; then
JOURNAL=-jsave bot"$a".journal
fi

for ((a=1; a <= LIMIT ; a++))  # Double parentheses, and "LIMIT" with no "$".
do
  echo Bot $a connecting to $1
  ./zapd -name bot"$a" "$JOURNAL" -loss 0.2 -lag 150 -crazybot -connect "$CONNECTADDR" &
  echo Waiting 10 seconds for bot to connect...
  sleep 4
done                           # A construct borrowed from 'ksh93'.

