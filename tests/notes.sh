#!/usr/bin/env bash


lli --entry-function=main

llc --filetype=obj --relocation-model=pic -o tests/helloworld.sood.{o,ll}

ld.lld --verbose -L /usr/lib -lc --entry main -o tests/helloworld{,.o}


###

reloc static?


##

llc --relocation-model=dynamic-no-pic tests/helloworld.sood.ll # -> tests/helloworld.sood.s
clang tests/helloworld.sood.s -o tests/helloworld # this works


/sbin/ld \
  -pie \
  --eh-frame-hdr \
  -m elf_x86_64 \
  -dynamic-linker \
  /lib64/ld-linux-x86-64.so.2 \
  -o tests/helloworld \
  `realpath /sbin/../lib64/gcc/x86_64-pc-linux-gnu/10.2.0/../../../../lib64/Scrt1.o` \
  `realpath /sbin/../lib64/gcc/x86_64-pc-linux-gnu` \
  `realpath /10.2.0/../../../../lib64/crti.o` \
  `realpath /sbin/../lib64/gcc/x86_64-pc-linux-gnu/10.2.0/crtbeginS.o` \
  -L/sbin/../lib64/gc \
  c/x86_64-pc-linux-gnu/10.2.0 \
  -L `/sbin/../lib64/gcc/x86_64-pc-linux-gnu/10.2.0/../../../../lib64` \
  -L/usr/bin/../l \
  ib64 \
  -L `realpath /lib/../lib64` \
  -L `realpath /usr/lib/../lib64` \
  -L `realpath /sbin/../lib64/gcc/x86_64-pc-linux-gnu/10.2.0/../../..` \
  -L `realpath /usr/bin/.` \
  ./lib \
  -L/lib \
  -L/usr/lib \
  /tmp/helloworld-e6c5f2.o \
  -lgcc \
  --as-needed \
  -lgcc_s \
  --no-as-needed \
  -lc \
  -lgcc \
  --as-neede \
  d \
  -lgcc_s \
  --no-as-needed \
  `realpath /sbin/../lib64/gcc/x86_64-pc-linux-gnu/10.2.0/crtendS.o` \
  `realpath /sbin/../lib64/gcc/x86_64-pc-` \
  `realpath linux-gnu/10.2.0/../../../../lib64/crtn.o`


# THIS WORKS!!!
ld --verbose -L/usr/lib -lc \
  -dynamic-linker \
  /lib64/ld-linux-x86-64.so.2 \
  /usr/lib/Scrt1.o \
  /usr/lib/crti.o \
  /usr/lib/gcc/x86_64-pc-linux-gnu/10.2.0/crtbeginS.o \
  /usr/lib/gcc/x86_64-pc-linux-gnu/10.2.0/crtendS.o  \
  tests/helloworld.sood.o \
  -o tests/helloworld \
  /usr/lib/crtn.o
