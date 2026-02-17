CFLAGS=-std=c11 -g -static
SRCS=$(filter-out foo.c tmp2.c,$(wildcard *.c))
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
	$(CC) -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	./test.sh
	rm -f 9cc *.o *~ tmp*

clean:
	rm -f 9cc *.o *~ tmp*

# 9ccで.sのファイルを作成
%.s: %.c 9cc
	./9cc $< > $@

# .sのファイルを.xにコンパイルする
%.x: %.s
	$(CC) -target x86_64-apple-darwin -o $@ $<

# ./xのファイルを実行する
%.run: %.x
	./$< || echo "Exit code: $$?"

.PHONY: test clean