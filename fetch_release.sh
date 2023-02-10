#!/bin/bash

git pull
git fetch release
git merge release/main -m "Merge release repository: "$1 --allow-unrelated-histories
git push origin main
