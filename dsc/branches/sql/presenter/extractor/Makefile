
LIBEXECS += store-incoming-data.pl
LIBEXECS += dsc-xml-extractor.pl
LIBEXECS += put-file.pl

ETCFILES  += dsc-extractor.cfg.sample

INSTALLDIR=/usr/local/dsc

all: ${LIBEXECS}
	@if perl -c -e 'use DSC::extractor;' 2>/dev/null ; then \
	for f in ${LIBEXECS} ; do \
		echo "perl -c $$f" ; \
		perl -c $$f ; \
	done \
	fi

install: all ${LIBEXECS}
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	install -d -m 755 $(INSTALLDIR)/libexec/
	install -m 755 $(LIBEXECS) $(INSTALLDIR)/libexec/
	install -m 644 $(ETCFILES) $(INSTALLDIR)/etc/

# In the trunk we use dsc-xml-extractor.pl but in the SQL branch
# we use store-incoming-data.pl.  Make one a symlink to the other.
#
dsc-xml-extractor.pl: store-incoming-data.pl
	ln -s store-incoming-data.pl $@

clean:
	rm -f dsc-xml-extractor.pl
