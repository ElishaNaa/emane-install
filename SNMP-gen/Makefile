#
# Warning: you may need more libraries than are included here on the
# build line.  The agent frequently needs various libraries in order
# to compile pieces of it, but is OS dependent and we can't list all
# the combinations here.  Instead, look at the libraries that were
# used when linking the snmpd master agent and copy those to this
# file.
#

CC=gcc

OBJS1=sendtrap.o
TARGETS=sendtrap

CFLAGS=-I. `net-snmp-config --cflags`
CXXFLAGS += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
BUILDLIBS=`net-snmp-config --libs`
BUILDAGENTLIBS=`net-snmp-config --agent-libs`

# shared library flags (assumes gcc)
DLFLAGS=-fPIC -shared

all: $(TARGETS)

sendtrap: $(OBJS1)
	$(CC) -o sendtrap $(OBJS1) $(BUILDLIBS) agent_trap.o

clean:
	rm $(OBJS2) $(OBJS2) $(TARGETS)

