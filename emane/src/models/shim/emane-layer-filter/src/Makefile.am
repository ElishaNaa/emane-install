lib_LTLIBRARIES = libemanelayerfilter.la

libemanelayerfilter_la_CPPFLAGS= \
 -I@top_srcdir@/include \
 $(libemane_CFLAGS) \
 -DBOOST_MPL_CFG_NO_PREPROCESSED_HEADERS \
 -DBOOST_MPL_LIMIT_VECTOR_SIZE=30 \
 -DBOOST_MPL_LIMIT_LIST_SIZE=30 \
 -DBOOST_TEST_DYN_LINK

libemanelayerfilter_la_SOURCES= \
 shimlayer.cc \
 filtermodemservice.cc 

EXTRA_DIST = \
 shimlayer.h \
 filtermodemservice.h 


libemanelayerfilter_la_LIBADD= \
 $(libemane_LIBS) \
 -lboost_system   \
 -lboost_thread	  

libemanelayerfilter_la_LDFLAGS= \
 -avoid-version
