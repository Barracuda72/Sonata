OBJS := main.o oms.o z_def.o xml.o
CFLAGS := -g -I. `xml2-config --cflags`
LIBS := -lz `xml2-config --libs`

# Windows hack
ifeq ($(OS), Windows_NT)
	LIBS += -lc -lcygwin
	TARGET := sonata.exe
else
	LINK := -lpthread -Xlinker --no-as-needed
	TARGET := sonata
endif

all: ver $(OBJS)
	gcc $(CFLAGS) -o $(TARGET) $(OBJS) $(LIBS) $(LINK)

clean:
	-rm $(OBJS) $(TARGET) version.h

ver: version.h.in
	cp version.h.in version.h
	svnversion -n >> version.h
	echo \" >> version.h
