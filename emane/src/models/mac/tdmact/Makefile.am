SUBDIRS=                             \
 .                                   \
 eventscheduler

lib_LTLIBRARIES = libtdmactbase.la

libtdmactbase_la_CPPFLAGS=             \
 -I@top_srcdir@/include              \
 $(AM_CPPFLAGS)                      \
 $(libemane_CFLAGS)                  \
 -std=gnu++1z

libtdmactbase_la_LIBADD=                       \
 $(libuuid_LIBS)                             \
 $(libxml2_LIBS)                             \
 $(protobuf_LIBS)                            \
 @top_srcdir@/src/libemane/.libs/libemane.la

libtdmactbase_la_LDFLAGS=              \
 $(AM_LDFLAGS)                       \
 -avoid-version

libtdmactbase_la_SOURCES =             \
 aggregationstatuspublisher.cc       \
 basemodel.cc                        \
 basemodelimpl.cc                    \
 basicqueuemanager.cc                \
 packetstatuspublisherimpl.cc        \
 pormanager.cc                       \
 queue.cc                            \
 queuestatuspublisher.cc             \
 receivemanager.cc                   \
 managementreceivemanager.cc         \
 slotstatustablepublisher.cc         \
 txslotinfosformatter.cc             \
 devlog.cc          			     \
 aggregationstatuspublisher.h        \
 basemodelimpl.h                     \
 basemodelmessage.h                  \
 basemodelmessage.inl                \
 managementmessage.h                 \
 managementmessage.inl               \
 packetstatuspublisherimpl.h         \
 pormanager.h                        \
 priority.h                          \
 queue.h                             \
 queuestatuspublisher.h              \
 receivemanager.h                    \
 managementreceivemanager.h          \
 slotstatustablepublisher.h          \
 txslotinfosformatter.h				 \
 devlog.h							 \
 rounder.h							 \
 rounder.inl

nodist_libtdmactbase_la_SOURCES =      \
 tdmactbasemodelmessage.pb.cc          \
 tdmactbasemodelmessage.pb.h		   \
 tdmactmanagementmessage.pb.cc     \
 tdmactmanagementmessage.pb.h

EXTRA_DIST=                            \
 tdmactbasemodelmessage.proto          \
 tdmactmanagementmessage.proto     \
 tdmactbasemodelpcr.xml

BUILT_SOURCES =                        \
 $(nodist_libtdmactbase_la_SOURCES)

clean-local:
	rm -f $(BUILT_SOURCES)

tdmactbasemodelmessage.pb.cc tdmactbasemodelmessage.pb.h: tdmactbasemodelmessage.proto
	protoc -I=. --cpp_out=. $<


tdmactmanagementmessage.pb.cc tdmactmanagementmessage.pb.h: tdmactmanagementmessage.proto
	protoc -I=. --cpp_out=. $<