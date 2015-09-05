
SUBDIRS = test-wads DOS Win32

man_MANS = bsp.6
EXTRA_DIST = visplane.txt README $(man_MANS)

DOCS = README AUTHORS COPYING NEWS visplane.txt

docdir = $(datadir)/doc/@PACKAGE@
doc_DATA = $(DOCS)

bin_PROGRAMS=bsp

bsp_SOURCES=blockmap.c bsp.c endian.c funcs.c level.c makenode.c picknode.c bsp.h structs.h

blockmap.c bsp.c funcs.c endian.c level.c makenode.c picknode.c : bsp.h structs.h config.h

dos	:	$(DOCS)
	-mkdir dos-dist
	for fn in $(DOCS); do perl -p -e 's/$$/\r/' < $$fn > `echo dos-dist/$$fn | perl -p -e 's/(.txt)?$$/.txt/'`; done

index.html	: /home/www/games/doom/bsp/index.html
	perl -pe '((/<div id="rightmenu">/ .. /<\/div>/) or (/<div id="footer">/ .. /<\/div>/)) and $$_="";' < $< |perl -pe '$$. == 2 and print "<base href=\"http://games.moria.org.uk/doom/bsp/\" />\n"' > $@

make-readme	: index.html	
	lynx -dump index.html > README

