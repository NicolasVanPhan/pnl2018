.PHONY : all clean check
.ONESHELL:

CHECK_PATCH=./checkpatch.pl --no-tree

all : testHistory

%.o : %.c
	gcc -Wall -c $< -g

testHistory : version.o commit.o history.o testHistory.o
	gcc -Wall $^ -o $@ -g

check : 
	for f in *.c *.h ; do
		$(CHECK_PATCH) -f $$f
	done

clean :
	rm -rf *.o testHistory
