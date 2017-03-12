#!/bin/bash
g++ -g /Users/davide/Documents/Steg/test/steg.cpp -o /Users/davide/Documents/Steg_test/dojo.test -Wall -ansi
git add .
git status
read -p "Commit description: " desc
git commit -m "$desc"
git push origin master
