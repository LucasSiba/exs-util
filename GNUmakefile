SXE_DEBUG ?= 0

CPPFLAGS  =     # include paths, '.' is implicit
CFLAGS    = -O2 -DSXE_DEBUG=$(SXE_DEBUG) -g -rdynamic -fstack-protector -fno-strict-aliasing -Wall -Werror -Wextra -Wcast-align -Wcast-qual -Wformat=2 -Wformat-security -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-prototypes -Wno-unknown-pragmas -Wunused -Wno-unused-result -Wwrite-strings -Wno-attributes
LDFLAGS   =     # linker options (like -L for library paths)
LDLIBS    = -lm # libraries to link with

all: exs-util.a

SOURCES  = exs-pool.c sxe-log.c tap.c
COVERAGE = exs-pool.c
TESTS    = test-exs-pool.c

exs-util.a: $(SOURCES:.c=.o)

%.a:
	[ "$^" ] && ar crs $@ $(filter %.o, $^)

test: $(TESTS:.c=.t.pass)

%.t: %.o exs-util.a
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.t.pass: %.t
	./$*.t > $*.t.fail 2>&1;  if [ $$? != 0 ] ; then cat $*.t.fail ; false ; else mv $*.t.fail $*.t.pass ; fi

debug: SXE_DEBUG = 1
debug: clean test

coverage: CFLAGS  += --coverage -O0
coverage: LDFLAGS += --coverage
coverage: clean test
	FILES='$(COVERAGE)' ; for f in $$FILES ; do gcov -b $$f | head -n4 ; egrep --color -A2 -B3 '(#####|branch .. taken 0.$$)' ./$$f.gcov ; true ; done

.PHONY: clean

clean:
	rm -f *.o *.a *.t *.pass *.fail *.gcov *.gcda *.gcno

.PRECIOUS: %.t

# n.o: n.c
# 	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
# n: n.o
#	$(CC) $(LDFLAGS) n.o $(LDLIBS)

# $@ - The file name of the target of the rule
# $< - The name of the first prerequisite
# $^ - The names of all the prerequisites, with spaces between them.
# $* - The stem with which an implicit rule matches

