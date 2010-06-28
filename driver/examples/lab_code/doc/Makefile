all: man html

man: alpha.1
html: alpha.html

alpha.xml: alpha.txt asciidoc.conf $(wildcard *.xsl)
	@asciidoc -b docbook -d manpage alpha.txt 2> /dev/null

alpha.1: alpha.xml
	@echo "    MAN alpha.1"
	@xmlto -m manpage-suppress-sp.xsl man alpha.xml 2> /dev/null
	@mv Alpha.1 alpha.1

alpha.html: alpha.txt
	@echo "   HTML alpha.html"
	@asciidoc -b xhtml11 -d manpage -f asciidoc.conf alpha.txt 2> /dev/null

clean:
	@rm -rf *.html *.xml *.1 doxygen

.PHONY: all clean
