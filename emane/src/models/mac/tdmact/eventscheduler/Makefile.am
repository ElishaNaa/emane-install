lib_LTLIBRARIES = libtdmacteventschedulerradiomodel.la

libtdmacteventschedulerradiomodel_la_CPPFLAGS= \
 -I@top_srcdir@/include              \
 $(AM_CPPFLAGS)                      \
 $(libemane_CFLAGS)

libtdmacteventschedulerradiomodel_la_LIBADD=   \
 $(libuuid_LIBS)                             \
 $(libxml2_LIBS)                             \
 @top_srcdir@/src/libemane/.libs/libemane.la \
 ../.libs/libtdmactbase.la

libtdmacteventschedulerradiomodel_la_LDFLAGS= \
 $(AM_LDFLAGS)                       \
 -avoid-version

libtdmacteventschedulerradiomodel_la_SOURCES = \
 radiomodel.cc                       \
 eventscheduler.cc                   \
 eventtablepublisher.cc              \
 eventscheduler.h                    \
 eventtablepublisher.h               \
 slotter.h                           \
 slotter.inl

EXTRA_DIST=                          \
 tdmactnem.xml.in                      \
 tdmactradiomodel.xml.in

BUILT_SOURCES =                      \
 tdmactnem.xml                         \
 tdmactradiomodel.xml

edit = sed                                \
       -e 's|@datadir[@]|$(pkgdatadir)|g'  

tdmactnem.xml: tdmactnem.xml.in
	$(edit) $< > $@

tdmactradiomodel.xml: tdmactradiomodel.xml.in
	$(edit) $< > $@

clean-local:
	rm -f $(BUILT_SOURCES)

install-exec-hook:	
	$(mkinstalldirs) $(DESTDIR)$(datadir)/$(PACKAGE)/xml/models/mac/tdmacteventscheduler
	cp -f tdmactnem.xml $(DESTDIR)$(datadir)/$(PACKAGE)/xml/models/mac/tdmacteventscheduler
	cp -f tdmactradiomodel.xml $(DESTDIR)$(datadir)/$(PACKAGE)/xml/models/mac/tdmacteventscheduler
	cp -f ../tdmactbasemodelpcr.xml $(DESTDIR)$(datadir)/$(PACKAGE)/xml/models/mac/tdmacteventscheduler

uninstall-local:
	rm -f $(DESTDIR)$(datadir)/$(PACKAGE)/xml/models/mac/tdmacteventscheduler/tdmactnem.xml
	rm -f $(DESTDIR)$(datadir)/$(PACKAGE)/xml/models/mac/tdmacteventscheduler/tdmactradiomodel.xml
	rm -f $(DESTDIR)$(datadir)/$(PACKAGE)/xml/models/mac/tdmacteventscheduler/tdmactbasemodelpcr.xml
