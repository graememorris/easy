#!/bin/bash

find "$1" -maxdepth 1 -mindepth 1 -not -name '.*' -printf "%P\t%Y\n" | sort -f | awk 'BEGIN{FS="\t"} {printf("<div>%s%s</div>", $1, ($2=="d" || $2=="l") ? "/" : "");}'
