#!/usr/bin/env bash
set -eu

_exec_file="$(cd "$(dirname "${BASH_SOURCE:-$0}")"; pwd)/ked"

if [ ! -x "$_exec_file" ]; then
    echo "Compile \`ked' beforehand." >&2

    exit 1
fi

LD_LIBRARY_PATH=libked exec "$_exec_file" --debug "$@"
