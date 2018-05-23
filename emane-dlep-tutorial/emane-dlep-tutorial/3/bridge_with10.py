#!/usr/bin/env python
from emanesh.events import EventService
from emanesh.events import PathlossEvent

# create the event service
service = EventService(('224.1.2.8',45703,'emanenode0'))

# create an event setting the pathloss between 1 & 10
event = PathlossEvent()
event.append(1,forward=91)
event.append(10,forward=91)

# publish the event
service.publish(1,event)
service.publish(10,event)

# create an event setting the pathloss between 9 & 10
event = PathlossEvent()
event.append(9,forward=91)
event.append(10,forward=91)

# publish the event
service.publish(9,event)
service.publish(10,event)

# create an event setting the pathloss between 1 & 2
event = PathlossEvent()
event.append(1,forward=91)
event.append(2,forward=91)

# publish the event
service.publish(1,event)
service.publish(2,event)

