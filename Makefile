PROJ=draw_sphere

CC=g++

CFLAGS=-Wall -g -DDEBUG

TINYXML_SRC = tinyxml/tinyxml.cpp tinyxml/tinystr.cpp \
tinyxml/tinyxmlerror.cpp tinyxml/tinyxmlparser.cpp

LIBS=-lglut

$(PROJ): $(PROJ).cpp colladainterface.cpp $(TINYXML_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

.PHONY: clean

clean:
	rm $(PROJ)
