
SCRIPTS += refile-and-grok.sh
SCRIPTS += daily-cleanup.pl.sample

INSTALLDIR=/usr/local/dsc

all:

install:
	@if test -n "$(INSTALLDIR)" ; then echo "installing in $$INSTALLDIR" ; else echo "set INSTALLDIR first"; false ; fi
	test -d $(INSTALLDIR)/libexec || mkdir $(INSTALLDIR)/libexec
	install -m 755 $(SCRIPTS) $(INSTALLDIR)/libexec/
	test -f ${INSTALLDIR}/libexec/daily-cleanup.pl || install -m 755 daily-cleanup.pl.sample ${INSTALLDIR}/libexec/daily-cleanup.pl

clean:
	
