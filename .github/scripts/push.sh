#!/bin/bash

git clone https://github.com/timothyschoen/PlugDataDekenServer.git

cd PlugDataDekenServer
rm -rf PlugDataDekenServer/xml
rm -rf PlugDataDekenServer/compressed

cp -rf build/xml PlugDataDekenServer/xml
cp -rf build/bin PlugDataDekenServer/compressed

git add -u
git commit -m "Daily repo update: $(date)"
git push