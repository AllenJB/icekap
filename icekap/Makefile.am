SUBDIRS = src images scripts
EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO

messages: rc.cpp
	$(EXTRACTRC) `find . -name "*.rc" -o -name "*.ui" -o -name "*.kcfg"` > rc.cpp
# we haven't got a tips file atm, so there isn't a lot of point in making one...
# maybe someone will document the process of writing tips files, or tell me that
# it needs to be done
#	$(PREPARETIPS) > tips.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name *.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		$(XGETTEXT) $$LIST -o $(podir)/icekap.pot; \
	fi
#	rm -f tips.cpp
	rm -f rc.cpp


include $(top_srcdir)/admin/Doxyfile.am

