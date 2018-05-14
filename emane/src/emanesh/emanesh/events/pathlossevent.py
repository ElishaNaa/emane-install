#
# Copyright (c) 2013-2014 - Adjacent Link LLC, Bridgewater, New Jersey
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in
#   the documentation and/or other materials provided with the
#   distribution.
# * Neither the name of Adjacent Link LLC nor the names of its
#   contributors may be used to endorse or promote products derived
#   from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

from . import Event
import pathlossevent_pb2

class PathlossEvent(Event):
    IDENTIFIER = 101

    def __init__(self):
        self._event = pathlossevent_pb2.PathlossEvent()

    def append(self,nemId,**kwargs):
        hasForward = False
        hasReverse = False

        pathloss = self._event.pathlosses.add()

        pathloss.nemId = nemId
        
        for (name,value) in kwargs.items():

            if name == 'forward':
                if isinstance(value,int) or \
                        isinstance(value,float):
                    hasForward = True
                    pathloss.forwardPathlossdB = value
                else:
                    raise ValueError("forward pathloss must be numeric")

            elif name == 'reverse':
                if isinstance(value,int) or \
                        isinstance(value,float):
                    hasReverse = True
                    pathloss.reversePathlossdB = value
                else:
                    raise ValueError("reverse pathloss must be numeric")

            else:
                raise KeyError("unknown parameter: %s" % name)

        if not hasForward:
            raise KeyError("must specify forward pathloss")
        elif not hasReverse:
            pathloss.reversePathlossdB = pathloss.forwardPathlossdB
            
    def serialize(self):
        return self._event.SerializeToString()

    def restore(self,data):
        self._event.ParseFromString(data)
        
    def __iter__(self):
        for pathloss in self._event.pathlosses:
            kwargs = {'forward': pathloss.forwardPathlossdB,
                      'reverse' : pathloss.reversePathlossdB}

            yield (pathloss.nemId,kwargs)

                
if __name__ == "__main__":
    import sys
    from optparse import OptionParser
    from emanesh.events import EventService

    usage = "emaneevent-pathloss [OPTION]... NEMID[:NEMID] PATHLOSSDB"

    description="Publish a pathloss event to all or some of the NEMs specified in a range."

    epilog="""
The NEM range specification creates a logical two dimensional matrix.

For example:

  emaneevent-pathloss 1:10 90

Will create the following logical matrix:

       Transmitters

       1  2  3  4  5  6  7  8  9  10
R   1     P  P  P  P  P  P  P  P  P
e   2  P     P  P  P  P  P  P  P  P
c   3  P  P     P  P  P  P  P  P  P
e   4  P  P  P     P  P  P  P  P  P
i   5  P  P  P  P     P  P  P  P  P
v   6  P  P  P  P  P     P  P  P  P
e   7  P  P  P  P  P  P     P  P  P
r   8  P  P  P  P  P  P  P     P  P
s   9  P  P  P  P  P  P  P  P     P
   10  P  P  P  P  P  P  P  P  P   

where, P is (pathloss=90).

Each NEM receives one event with its respective matrix slice
containing the pathloss value to use for each respective
transmitter.

This example will publish 10 pathloss events, each event will target
a specific NEM in the range [1,10].

You can use the '--target' option and the '--reference' to specifically
target one or more NEMs with one of more of the transmitter values. The
NEMs specified with '--target' do not have to be in the range.

For example:

  emaneevent-pathloss 1:10 90 -t 3 -t 4 -r 8 -r 9 -r 10

Will send events to NEM 3 and 4 containing the transmitter information
for NEMs 8, 9 and 10.

  emaneevent-pathloss 13 90 -t 7 -t 8

will send a pathloss event to NEM 7 and 8 with a single entry of 90dB
for packets transmitted by NEM 13.

  emaneevent-pathloss 15 110 -t 0

will send a pathloss event to all NEMs with a single entry of 110dB
for packets transmitted by NEM 15.

Asymmetric pathloss can be achieved my invoking emaneevent-pathloss
multiple times with different targets.

"""

    class LocalParser(OptionParser):
        def format_epilog(self, formatter):
            return self.epilog

    optionParser = LocalParser(usage=usage,
                               description=description,
                               epilog=epilog)

    optionParser.add_option("-p", 
                            "--port",
                            action="store",
                            type="int",
                            dest="port",
                            default=45703,
                            help="Event channel listen port [default: %default]")

    optionParser.add_option("-g", 
                            "--group",
                            action="store",
                            type="string",
                            dest="group",
                            default="224.1.2.8",
                            help="Event channel multicast group [default: %default]")

    optionParser.add_option("-i",
                            "--device",
                            action="store",
                            type="string",
                            dest="device",
                            help="Event channel multicast device")

    optionParser.add_option("-t",
                            "--target",
                            action="append",
                            type="int",
                            dest="target",
                            help="Only send an event to the target")

    optionParser.add_option("-r",
                            "--reference",
                            action="append",
                            type="int",
                            dest="reference",
                            help="Send events to targeted NEMs but only include information for the reference NEM.")


    (options, args) = optionParser.parse_args()

    service = EventService((options.group,options.port,options.device))

    if len(args) < 2:
        print >>sys.stderr,"missing arguments"
        exit(1)

    nems = args[0].split(':')

    if len(nems) == 0 or len(nems) > 2:
        print >>sys.stderr,"invalid NEMID format:",args[0]
        exit(1)

    try:
        nems = [int(x) for x in nems]
    except:
        print >>sys.stderr,"invalid NEMID format:",args[0]
        exit(1)

    if len(nems) > 1:
        nems = range(nems[0],nems[1]+1)

    if not nems:
        print >>sys.stderr,"invalid NEMID format:",args[0]
        exit(1)

    pathlossdB = args[1]
    
    try:
        pathlossdB = float(pathlossdB)
    except:
        print >>sys.stderr,"invalid pathloss format:",args[1]
        exit(1) 

    if nems[0] == 0:
        print >>sys.stderr,"0 is not a valid NEMID"
        exit(1)


    if options.target:
        targets = options.target
    else:
        targets = nems

    if options.reference:
        references = options.reference
    else:
        references = nems

    for i in targets:
        event = PathlossEvent()
        for j in nems:
            if i != j and j in references:
                event.append(j,forward=pathlossdB)

        service.publish(i,event)
