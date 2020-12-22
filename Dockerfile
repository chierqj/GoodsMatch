FROM lycantropos/cmake

RUN  mkdir /usr/src/myapp

ADD data/code/GoodsMatch /usr/src/myapp/GoodsMatch
WORKDIR /usr/src/myapp/GoodsMatch/script

RUN chmod +x *.sh
CMD ["./build_and_run.sh"]