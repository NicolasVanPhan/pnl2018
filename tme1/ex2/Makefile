.PHONY : all clean check
.ONESHELL:

CHECK_PATCH=./checkpatch.pl --no-tree

all : testOffset

%.o : %.c
	gcc -Wall -c $< -g

testOffset : version.o commit.o testOffset.o
	gcc -Wall $^ -o $@ -g

check : 
	for f in *.c *.h ; do
		$(CHECK_PATCH) -f $$f
	done

clean :
	rm -rf *.o testOffset
