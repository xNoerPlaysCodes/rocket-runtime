#!/bin/zsh

if [[ -z "$1" ]]; then
    echo "Usage: $0 [exe name]  (e.g. ui_init_test)"
    exit 1
fi

if [[ -f "bin/tests/$1" ]]; then
    (cd bin && tests/"$1" ${@:2})
elif [[ -f "bin/examples/$1" ]]; then
    (cd bin && examples/"$1" ${@:2})
else
    echo "couldn't find!"
    exit 1
fi
