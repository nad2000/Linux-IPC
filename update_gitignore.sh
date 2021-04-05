#!/bin/env sh

# add executales to the .gitignore
for f in $(find . -perm /111 -type f | sed 's#^./##' | sort -u); do grep -q "$f" .gitignore || echo "$f" >> .gitignore ; done

