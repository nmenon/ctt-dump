SRC:=ctt-dump.c
OUT:=ctt-dump
all:
	$(CROSS_COMPILE)gcc -o $(OUT) -I. --static -Wall $(SRC)
	$(CROSS_COMPILE)strip $(OUT)

sandbox:
	$(CROSS_COMPILE)gcc -ggdb -D CONFIG_SANDBOX -o $(OUT) -I. --static -Wall $(SRC)

clean:
	rm -f $(OUT)

distclean: clean

