#!/bin/bash

trace_files=$(find ./slink/scripts -name *.txt)
for file in $trace_files; do
  echo "############ $file ############"
  ./stsh-driver -t $file -s ./slink/stsh_soln -a "--suppress-prompt --no-history" > /tmp/stsh_soln.txt 2>&1
  ./stsh-driver -t $file -s ./stsh -a "--suppress-prompt --no-history" > /tmp/stsh.txt 2>&1
  diff /tmp/stsh_soln.txt /tmp/stsh.txt
done