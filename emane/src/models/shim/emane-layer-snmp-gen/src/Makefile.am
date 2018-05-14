lib_LTLIBRARIES = libemanelayersnmp.la

libemanelayersnmp_la_CPPFLAGS= \
 -I@top_srcdir@/include \
 $(libemane_CFLAGS) \
 -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS \
 -DBOOST_MPL_LIMIT_VECTOR_SIZE=30 \
 -DBOOST_MPL_LIMIT_LIST_SIZE=30 \
 -DBOOST_TEST_DYN_LINK

libemanelayersnmp_la_SOURCES= \
 shimlayer.cc \
 snmpmodemservice.cc \
 dlepMac.cc

EXTRA_DIST = \
 shimlayer.h \
 snmpmodemservice.h \
 dlepMac.h


libemanelayersnmp_la_LIBADD= \
 $(libemane_LIBS) \
 -lboost_system   \
 -lboost_thread	  \
 -lhiredis

libemanelayersnmp_la_LDFLAGS= \
 -avoid-version
