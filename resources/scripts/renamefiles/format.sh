#!/bin/bash

set -e
for js_file in *.js; do
    js-beautify --jslint-happy -r "$js_file"
done
