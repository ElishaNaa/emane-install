NB: The documentation is admittedly not very organic. But if you are new go to the end, to the "Implementation" section, to see the overall look on things (classes, functions, ...), and see the "TODO" section to see what is left to do.







# Architecture of the scheduler

## Fast things
Command to run things up

emane "platform.xml" -l 3 -f "persist/host/var/log/emane.log" --pidfile "persist/host/var/run/emane.pid" --uuidfile "persist/host/var/run/emane.uuid"

for logging
cp tdmactnem1.xml tdmactnem6.xml && sed -i s'/persist\/1/persist\/6/' tdmactnem6.xml 


## Requirements
*Frames*: (aka in Eugeni's terms, a *frame*)
Only one NEM (later, two) can transmit and all the others receive

*Retransmission slots*:
Inside each time slot a transmitter transmits in broadcast in a shorter interval
(retransmission slot), all the NEMS that receive it retransmit it in the next 
retransmission slot, and so on for `retransmissionslots` retransmissions (default: 4)

*Schedule*
Each NEM mantains a schedule of who will be the transmitter in the next `numscheuduledslots` and the priority associated with its transmission. This schedule can be reset externally, via an event, or between the NEMS, via the scheduling race (see below).

*Scheduling race*:
Between any two slots a race takes place to decide the schedule. Each NEM sends a number composed of 7 pseudorandom bits and 3 priority bits (the urgency that it has to get a slot).
This transmission is also a retransmission in `priorityretransmissionslots` steps (default: 6).
They compare between themselves the numbers. The winner is the one with the highest priority, or the one with the highest random number.
The winner knows that it has won and then sends a schedule of its own wish, via `scheduleretransmissionslots` retransmissions (default: 6), to the other NEMs. It can occupy each slot that it wishes and assign it to itself (this is what we implemented for now). 


`

#### Original implementation
*Receiving*
When a message of the type `BaseModelMessage` has been processed by the physical layer (and has been accepted) processUpstreamMessage is called. The procedure checks if the timing of the message is correct, in particular if
- The packet is well formed (its length is the expected length, etc..)
- The message should have arrived physically in the same slot it was transmitted in. The absolute time slot and the retransmission time slot that is written in the message (see `basemodelmessage.proto`) correspond to the slot that contains `txTime + propagation`, where `txTime` is in the packet header and is programmed to be always the start of the retransmission slot when the message is sent.
- The time slot of NOW is not a TX slot for the receiving NEM.
- The time slot of NOW has not already been used to retransmit a received message



If it is all well it has to be chcked if the message passes the noise floor according to the SINR calculation and random number evaluation. This is done by the `ReceiveManager` class. 



and there are still retransmission slots, the message will be scheduled to be retransmitted in the following retransmission slot via copying the message, changing its `relativeretransmissionslot` field and packing it with the method `retransmit` and sending the whole package to the scheduler.



*Sending*
(old)
When the event of a new schedule is received, we schedule a `processTxOpportunity`, that schedules itself for the next opportunity and so on. During the opportunity it calls `sendDownStreamPacket` that get the packets from the queue and sends them to the physical layer with a TimeStamp control (that will decide the txTime of the packet).

check for time?
cancel schedule?
(new)
The start of each time of the slot is .


### Timing



*State variables*
`pendingReceiveRetransmissionSlot` (optional, when there is still)
`pendingReceiveManagementRetransmissionSlot` (optional, )
`pendingSlot` (optional, ...)
`pendingSelfManagement` (connected to a slot..) 
`pendingReceivedManagement` (optional, current maximum)
`pendingSchedule` (TimedSchedule containing start of schedule and schedule itself) 
`pendingInfo` (message to process)
`pendingManagementInfo` (management to process)

## Overall architecture of slot/frame/part processing
*Time slot start*

-> processReceivalTimeSlotEnd
-> processManagementReceivalTimeSlotEnd

Check now vs expected slot:
< error "now is before expected slot", change expected slot
> error "missed slot", change expected slot
== ok, continue

Elaborate schedule (info message if there is no schedule):

Check which transmitter on schedule:
id_ == transmitter(schedule) -> 
   check retransmissionslot(now) 
   != error "missed transmission"
   == transmit message

id_ != transmitter(schedule) ->
   schedule next retransmission subslot check



*Management start*
Check now vs expected slot
< error "now is before expected slot", change expected slot
> error "missed slot", change expected slot
== ok, continue

check retransmissionslot(now) 
   != error "missed management transmission"
   == transmit my message

schedule process 1,2,3,4,5


*Receival of message*
Check vs now, and propagation ecc. (in processUpstream)

Enqueue the message


*Receival of management*
Check vs now, and propagation ecc. (in processUpstream)

Enqueue the management message


*Enqueue message*
Possibly process receival subslot end?

Check SINR?
>= Retransmit
<
   Numleft
     Schedule next process


*Enqueue management*
Possibly process receival management retransmissionslot end?


*Process receival retransmissionslot end*
Check expected retransmissionslot, and if it is not correct it does nothing

Basically it should send the message to the receivemanager (an `enqueue` inside it and a schedule of `process`)

Retransmit and then not schedule intermediate processes again

Join it to the start of the management slot? Actually there is no need for that, so I'll leave the start of the management slot and the end or the message slot to be different notifiable functions

Check retransmissionslot:
- last: Save the actual schedule from ORing all the last received messages
- not last: Retransmit the message that results from ORing the received messages and myself.


*Process receival time-slot end*
Here there is the need for the good old ReceiveManager, because of its functions that manage defragmentation (that I have no intention to look into :)

All the timings and checking will be done from the BaseModel, so that part in ReceiveManager has been removed, and ReceiveManager will become actually an accumulator for messages with three important methods
- `startSlot`
- `enqueue`
- `conclude`

So this will be the process inside the receivemanager, but the part with the sinr will have to be removed


*Process management receival retransmissionslot end*
Check expected retransmissionslot, and if it is not correct it does nothing

Checks the vector of received messages upon the spectrum analyzer (via the receivemanager)

Check retransmissionslot:
- last: Save the actual schedule ORing all the last received messages
- not last: Retransmit the message that results from ORing the received messages and myself.


*Process management receival time-slot end*
Save changes to schedule.


### Schedule
In the original TDMA Emane implementation a NEM sees a slot as a time interval that can be TX (transmit), RX (receive) or IDLE. It receives as an event the structure of a multiframe which is a sequence of slots associated with type (TX/RX/IDLE), frequency, datarate, class, etc.. that is periodically repeated beginning with the Unix epoch. Each time slot is of fixed size so it is very simple to covert the current time into the absolute index of the current slot (see the `Slotter` class).
Also, in the original emane's TDMA there is a python script, `emaneevent-tdmaschedule` whose objective is to convert a schedule xml to events that are dispatched to all the NEMs.
In TDMACT a schedule is a map of slots to a structure which contains the transmitting NEM in that slot, the priority of that slot and a congestion parameter. A message containing the schedule is dispathed by the NEM that has won the priority race as a `WinnerScheduleMessage`, that is a regular message with a specific index in the `CommonMACHeader` header and with the structure in `winnerschedulemessage.proto`.
When a NEM receives a `WinnerScheduleMessage` in the correct time it will use it. If it receives two winnerschedulemessages in the same time slot it will be a serious error (ASK what to do) $e^2$

### Priority race

### Message priority

### Winner algorithm
The winner will always delete irrelevant (elapsed or missed) slots, then replace the first slot that has a lower priority with a slot of its own, with the priority it had when transmitting self-priority messages. The real radio we are simulating does not work exactly like that but in the meantime this is the algorithm we are using.
















------ FROM HERE DOCUMENTATION IS TERSE BUT CLEAR ----

## Implementation

### Transmission of messages
A message that has to be sent (and comes from the transport layer) goes to `processDownstreamPacket` that adds it to a queue (via the queuemanager). When the timing is right the packet will be sent by one of the following:

`sendDownstreamPacket`
send a packet from the queue. The first transmission is from this function (that also updates statistics). The packet comes from a queue, via the queuemanagerl

`sendDownstreamMessage`
send a message. All retransmissions of messages use this function (that does not currently update statistics). The packet comes from the receive manager (who knows what packet to resend)

`sendDownstreammanagement`
send a management message. All transmission (first and following) of management messages use this function (that does not currently update statistics). The packet comes from the management receive manager (who knows what management packet to resend)

### Time interval processing

`notifyScheduleChange`
activates when it receives a well-formed xml scheduling event (the xml scheduling events are deprecated but this is just to start the engine, no connection to the content)

`processFrameBegin`
process the beginning of a frame (and the end of the previous one) 

`processRetransmissionSlotBegin`
process the beginning of a retransmission slot  (and the end of the previous one) 

`processManagementPartBegin`
process the beginning of the management part  (and the end of the previous retransmission slot) 

`processManagementRetransmissionSlotBegin`
process the beginning of a mangement retransmission slot  (and the end of the previous one) 

     

### Message receival

`processUpstreamPacket`
when the MAC receives something. The function then calls one of the following depending on the type of message:

`processUpstreamPacketBaseMessage`
receive a message packet. Do some logic. Enqueue it in the receive manager

`processUpstreamPacketManagement`
receive a management packet. Do some logic. Enqueue it in the management receive manager


### Class structure
- `BaseModelImplementation`
the most important class, where most of the logic is contained

- `ReceiveManager` , `ManagementReceiveManager`
classes that are basically wrappers for data structures of messages received during the frame or management messages received during the management part

- `EventScheduler` (only user of `Slotter` and `EventTablePublisher`)
this is a *discontinued* class that was used in the original emane TDMA implementation to distribute a schedule. Here it is still used to start the TDMACT engine. A receival of a well-formed schedulke will just start frames and so on, without any connection whatsoever to the content of the schedule

- `Rounder`
for rounding

- `BaseModelMessage`
a wrapper around a basemodel packet (basically a packet, that can contain multiple subpackets in accordance with fragmentation and aggregation)

- `ManagementMessage`
basically a wrapper around the Google Protobuf implementation of a Management message
it has three fields for the probability
and some control information

- `CTLogger` (in devlog.h)
to write in the development log (a pipe)

- `BasicQueueManager`, `Queue`
queue for transmitting

- `PORManager`
to parse the SINR/POR xml file

- `*Publisher`
to collect statistics on *

- date.h
Just some utility functions to better print TimePoints. Useful for quick corrections that need logs. But probably it can be removed from the project

## TODO

###
- Check what happens when a "slot/part/retransmission/management retransmission" is missed (what happens to the data structure in the receive managers ?)

The affected functions are:
`processFrameBegin`
`processRetransmissionSlotBegin`
`processManagementPartBegin`
`processManagementRetransmissionSlotBegin`

How to correct it:
The logic should be that this is a serious simulation error (the simulation will not be sound because of this). Any missing of any type should make the NEM wait for the next management part (no reception no sending message till that point). An error flag (which should have a lock on it because there is access from 2 thread) should be raised at the first missing and should be removed when a management part is started correctly in the future. Also `ReceiveManager` and `ManagementReceiveManager` should be cleared with the `clear()` method (the vector that is actually cleared is `pendingInfos_`).

How to verify it:
Write statistics to emane shell. The publisher classes already do this and we need to base upon `SlotStatusTablePublisher` class ancd change it (simplify it a bit), to print the percentage of frames where a missing error occured (it is just a single number).
In order to see the error in action it is possible to freeze/unfreeze the containers/machines.

###
- Collaborative transmission

It is a feature that should be added so that many identical packets that arrive at the same time should help each other. All the idea of SINR should change to reflect it. The changes will be probably mostly in the PHY layer (that manages pathlosses and TX/RX powers), and in the classes like `SpectrumMonitor` and `SpectrumWindowUtils` that manage the interferences.

###
- Bitrate
Explanation: the bitrate in the xml's should be around 96000 bps, now it is 192000 bps

The idea is that the bitrate is the "real" bitrate.

For example, from the next paramenters:

  <param name='retransmissionslots' value='4'/>
  <param name='managementretransmissionslots' value='6'/>
  <param name='frameduration' value='200000'/>
  <param name='managementduration' value='10000'/>
  <param name='managementeffectiveduration' value='100000' />
  <param name='datarate' value='192K'/>

A retransmission slot in maban should be 
T = ( 200000 [`frameduration`] - 10000 [`managementduration`]) / 4 [`retransmissionslots`]   microseconds

so the MTU should be T*`datarate`

In our implementation the management part takes much more time, exactly 100000 microseconds [managementeffectiveduration]

So the software adapts the datarate to match an effectivedatarate (accessible by `getEffectiveDataRateBps`) that provides the same MTU to slots that are now sized 25 microseconds (instead of almost 50000).

###
- Check again the logic where it does not resend more then once a message it received. Meaning, if you receive a message 

line in receivemanager.cc:
```
      if(!pendingReceived_) {
        pendingReceived_ = std::move(i);
        if(!toReturn) {
          toReturn = std::get<0>(*pendingReceived_);
        }
      }
```

check in method `processUpstreamPacketBaseMessage`:
```
          if (baseModelMessage.getAbsoluteFrameIndex() > lastReceptionAbsoluteFrame_)
                ... receive message...
          else
            CTLOG("NEM %d message already received in this frame \n", id_);
```

It does work now but is there really a need for two checks? The check inside the receivemanager seem superfluous.

###
- Check what happened to the flow control.

During development I erased and ignored all references to flow control.

###
- Promiscuous mode off should not block retransmission, only upstream

Not sure if the error still happens. It showed itself as the system not working when the enablepromiscuousmode was off in the xml so to check it reset to off and see if it works.


###
- Verify copying operations that can be substituted by "move" or pointers (for speed) and if the operations are correct in terms of scope and object ownership (to avoid memory errors)

###
- Verify if locks can be minimized in scope

###
- Rewrite sendDownstreamPacket to use sendDownstreamMessage (and avoid code duplication)

There are two functions now that send a packet (a message) from the MAC downstream to the phy depending if it is the first sending (sendDownstreamPacket) or a retransmission (sendDownstreamMessage). It is possible to remove the need to use sendDownstreamPacket

###
- Cleaning up the code (removing scheduler, reorganizing)

The EventScheduler is a legacy function for receivent scheduling "events" in real time from the user.

Now the only use of it is to start the TDMACT timings (`notifyScheduleChange`). But it all can be removed (the start can be moved to the function postStart() ) or extended (if there is the need to send messages to the system in real time).

###
- Verify `notifyScheduleChange`
It does not clear receivemanager and managementreceivemanager now. But what happens if it receives messages before receiving the schedule, that starts the timings?

###
- Refactor in code some refernces to "frame" which is now called "slot"
- Refactor second bits in priority "congestion" to "occupation"

###
- Documentation




## Less important wishlist
- change `retransmissionslots` at runtime (via event) [why?]