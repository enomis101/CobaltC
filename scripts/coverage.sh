#!/bin/bash

cd build
make coverage
xdg-open coverage_html/index.html