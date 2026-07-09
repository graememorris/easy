cc=gcc
cxx=g++
cflags=-O2 -Wall
cxxflags=-O2 -Wall

anim2db: src/anim2db.c src/getanim.c
	$(cc) $(cflags) $^ -lsqlite3 -lavformat -lavcodec -lavutil -o bin/$@

anim2db2: src/anim2db02.c src/getanim.c
	$(cc) $(cflags) $^ -lsqlite3 -lavformat -lavcodec -lavutil -o bin/$@

tagextractor: src/tag_executor.c src/tag_cleanser.cpp
	$(cxx) $(cxxflags) -c src/tag_cleanser.cpp -o bin/tag_cleanser.o
	$(cc) $(cflags) src/tag_executor.c bin/tag_cleanser.o -ltag -lstdc++ -o bin/$@

all: bin anim2db anim2db2 tagextractor
