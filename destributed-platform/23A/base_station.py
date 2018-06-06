import os
import time
from emanesh.events import EventService
from emanesh.events import LocationEvent
from emanesh.events import PathlossEvent

#base station Haifa
#elvation 457
#lat 32.7625
#lon 35.0195

#haifa  airport
#elevation 8
#lat 32.8119
#lon 35.0433


#Naharia
#elevation 1000
#lat 33.005811
#lon 35.099478

#Naharia GDUD
#elevation 8
#lat 33.010763
#lon 35.088425

#Tiberias
#elevation 1000
#lat 32.794592
#lon 35.532089

#sdtcmd_path = "/home/core/Downloads/sdt-linux-amd64-2.1/sdtcmd"
sdtcmd_path = "/home/ubuntu/Software/sdt-linux-amd64-2.1/sdtcmd"

# def Distance(lat1, lat2, lon1, lon2):
#     R = 6371 # km
#     dLat = toRad(lat2 - lat1)
#     dLon = toRad(lon2 - lon1)
#     a = Math.sin(dLat / 2) * Math.sin(dLat / 2) + Math.cos(toRad(lat1)) * Math.cos(toRad(lat2)) * Math.sin(dLon / 2) * Math.sin(dLon / 2)
#     c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a))
#     d = R * c * 0.621371
#     r = Math.round(d * 100) / 100
#     return r

def init_nodes(service):
    # os.system(sdtcmd_path + " " + "reset")
    os.system(sdtcmd_path + " " + "sprite base_station image base_station.png size 52,25")
    os.system(sdtcmd_path + " " + "node node-01 type base_station label blue")
    os.system(sdtcmd_path + " " + "node node-01 position 35.0195,32.7625")
    os.system(sdtcmd_path + " " + "sprite plane image plane.png size 52,25")
    os.system(sdtcmd_path + " " + "node node-02 type plane label blue")
    os.system(sdtcmd_path + " " + "node node-02 position 35.0433,32.8119")
    os.system(sdtcmd_path + " " + "link node-01,node-02,wifi")
    os.system(sdtcmd_path + " " + "sprite soldier image soldier.gif size 52,25")
    os.system(sdtcmd_path + " " + "node node-03 type soldier label blue")
    os.system(sdtcmd_path + " " + "node node-03 position 35.088425,33.010963")
    os.system(sdtcmd_path + " " + "node node-06 type soldier label blue")
    os.system(sdtcmd_path + " " + "node node-06 position 35.147439,33.015844")
    # create an event setting base station position
    event = LocationEvent()
    event.append(11, latitude=32.7625, longitude=35.0195, altitude=457.000000)
    service.publish(0, event)
    # create an event setting plane position
    event = LocationEvent()
    event.append(12, latitude=32.8119, longitude=35.0433, altitude=500.000000)
    service.publish(0, event)
    event = LocationEvent()
    event.append(7, latitude=32.8119, longitude=35.0433, altitude=500.000000)
    service.publish(0, event)

    # set 1 men position
    event = LocationEvent()
    event.append(1, latitude=33.010763, longitude=35.088425, altitude=8.00000)
    service.publish(0, event)
    event = LocationEvent()
    event.append(2, latitude=33.010863, longitude=35.088425, altitude=8.00000)
    service.publish(0, event)
    event = LocationEvent()
    event.append(3, latitude=33.010963, longitude=35.088425, altitude=8.00000)
    service.publish(0, event)
    event = LocationEvent()
    event.append(8, latitude=33.010963, longitude=35.088425, altitude=8.00000)
    service.publish(0, event)
    # set 2 men position
    event = LocationEvent()
    event.append(4, latitude=33.015644, longitude=35.147439, altitude=8.00000)
    service.publish(0, event)
    event = LocationEvent()
    event.append(5, latitude=33.015744, longitude=35.147439, altitude=8.00000)
    service.publish(0, event)
    event = LocationEvent()
    event.append(6, latitude=33.015844, longitude=35.147439, altitude=8.00000)
    service.publish(0, event)
    event = LocationEvent()
    event.append(9, latitude=33.015844, longitude=35.147439, altitude=8.00000)
    service.publish(0, event)
    time.sleep(20)


def move_node(service, coordinates):
    for i in range(0, len(coordinates)):
        if i == 0:
            continue
        for counter in range(0, 2000):
            counter = float(counter)/2000
            lon = coordinates[i-1][0] + counter * (coordinates[i][0] - coordinates[i-1][0])
            lat = coordinates[i-1][1] + counter * (coordinates[i][1] - coordinates[i-1][1])
            os.system(sdtcmd_path + " " + "node node-02 position" + " " + str(lon)[0:7] + "," + str(lat)[0:7])
            os.system(sdtcmd_path + " " + "wait 50.0")
            # create an event setting plane position
            event = LocationEvent()
            event.append(12, latitude=lat, longitude=lon, altitude=500.000000)
            service.publish(0, event)
            event = LocationEvent()
            event.append(7, latitude=lat, longitude=lon, altitude=500.000000)
            service.publish(0, event)
            time.sleep(0.01)
        time.sleep(20)



if __name__ == "__main__":
    #haifa airport, Naharia, Tiberias, Haifa airport
    coordinates = [( 35.0433, 32.8119), (35.121314, 33.017947), (35.532089, 32.794592), (35.0433, 32.8119)]
    service = EventService(('224.1.2.8', 45703, 'emanenode0'))
    init_nodes(service)
    move_node(service, coordinates)
    print "Done"
