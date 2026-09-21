#!/bin/sh
set -eu

echo "=== Linux Discovery Lab ==="
echo "USER:"
id
echo "SYSTEM:"
uname -a
echo "PROCESS SAMPLE:"
ps -ef | head -n 15
echo "LAB FILES:"
find ./lab -type f