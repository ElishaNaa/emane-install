#!/bin/bash
#This is a bash script, pay attention that you run in the right way. (/bin/bash).

#This script is getting the parameter to being SET and the parameter value.
#The script can handle ONLY with networkid (TODO frequency and power) parametr update.
#Documentation on onenote.
#Publish event with the new schedule.

#set power:
#open the schedule, go to the specific node, remove close-tag, on the next line write,
#<tx power='val'/>
#on the next line, close the slot tag.

PARAM="$1"
PARAMVAL="$2"

getId()
{
	id=$(tr -d "node-" <<< $(hostname))
	echo $id
}

ID=$(getId)

if [ "$PARAM" = "networkid" ]; then
	#kill emane
	echo cisco | sudo -S pkill emane

	#change subid value
	xmlstarlet ed --inplace -u "nem/phy/param[@name='subid']/@value" -v $PARAMVAL /home/user/ELBIT/emane-tutorial/snmpTDMA/tdmanem$ID.xml

	#run emane
	cd /home/user/ELBIT/emane-tutorial/snmpTDMA/; sudo emane platform$ID.xml -r -d -l 3 -f \
	/home/user/ELBIT/emane-tutorial/snmpTDMA/persist/$ID/var/log/emane.log \
	--pidfile /home/user/ELBIT/emane-tutorial/snmpTDMA/persist/$ID/var/run/emane.pid \
	--uuidfile /home/user/ELBIT/emane-tutorial/snmpTDMA/persist/$ID/var/run/emane.uuid
	#sleep 1
	emaneevent-tdmaschedule /home/user/ELBIT/emane-tutorial/snmpTDMA/schedule.xml -i eth1
	emaneevent-pathloss 1:3 9 -i eth1
fi
