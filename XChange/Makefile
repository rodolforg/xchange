all: libxchange-0.1.a

OBJS=src/file.o src/chartable.o

libxchange-0.1.a: $(OBJS)
	ar rcs $@ $?

clean:
	rm -f $(OBJS)
