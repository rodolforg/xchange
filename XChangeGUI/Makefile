PROGRAM=xchange

CFLAGS=`pkg-config --cflags gtk+-3.0 gmodule-export-2.0` -I ../XChange/src/
LDFLAGS=-L ../XChange
LDLIBS=`pkg-config --libs gtk+-3.0 gmodule-export-2.0` -lxchange-0.1

OBJS=src/filehandler.o src/hexview.o src/main.o src/message_dialogs.o src/preferencias.o

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LOADLIBES) $(LDLIBS)

clean:
	rm -f $(OBJS) $(PROGRAM)
