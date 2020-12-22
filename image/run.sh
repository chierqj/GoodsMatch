# /bin/bash
cd "$(dirname "$0")"
sh build.sh
cd ../bin
./GoodsMatch ../config/GoodsMatch.ini
