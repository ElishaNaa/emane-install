#!/bin/bash

ipaddressredis=$1
PLACE=".1.3.6.1.4.1.16215.1.24.1.4"
getId()
{
	id=$(tr -d "modem-" <<< $(hostname))
	echo $id
}

listen=$1
id=$(getId)

for (( i=8; i<=14; i++ ))
do
        
#Frequency
  if [ $id$PLACE.1.$i = $id$PLACE.1.8 ]; then
      VALUE=225300  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.1.$i $VALUE)   
  fi
#Power
  if [ $id$PLACE.1.$i = $id$PLACE.1.9 ]; then
      VALUE=1  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.1.$i $VALUE)   
  fi
#BlackStopTx
  if [ $id$PLACE.1.$i = $id$PLACE.1.10 ]; then
      VALUE=0  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.1.$i $VALUE)
  fi
#Network Id
  if [ $id$PLACE.1.$i = $id$PLACE.1.11 ]; then
      VALUE=1  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.1.$i $VALUE)
  fi
#TDMA Cycle Type
  if [ $id$PLACE.1.$i = $id$PLACE.1.12 ]; then
      VALUE=10 
      tmp=$(redis-cli -h $listen  SET $id$PLACE.1.$i $VALUE)
  fi
#ControlLoopPriority
  if [ $id$PLACE.1.$i = $id$PLACE.1.13 ]; then
      VALUE=2
      tmp=$(redis-cli -h $listen  SET $id$PLACE.1.$i $VALUE)  
  fi
#PacketLifeTime
  if [ $id$PLACE.1.$i = $id$PLACE.1.14 ]; then
      VALUE=25  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.1.$i $VALUE)        
  fi
done

#BlackStopTx
tmp=$(redis-cli -h $listen  SET $id$PLACE.2.1 1)  

for (( i=3; i<=12; i++ ))
do
        
#Calendar Time of Network
  if [ $id$PLACE.$i = $id$PLACE.3 ]; then
      VALUE="UTC"  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)   
  fi
#Algorithmic Time of network
  if [ $id$PLACE.$i = $id$PLACE.4 ]; then
      VALUE="NTP"  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)   
  fi
#Router Load
  if [ $id$PLACE.$i = $id$PLACE.5 ]; then
      VALUE=3  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)
  fi
#Data Rate
  if [ $id$PLACE.$i = $id$PLACE.6 ]; then
      VALUE=100  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)
  fi
#BytesInTxQueue
  if [ $id$PLACE.$i = $id$PLACE.7 ]; then
      VALUE=10 
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)
  fi
#PacketsInTxQueues
  if [ $id$PLACE.$i = $id$PLACE.8 ]; then
      VALUE=2
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)  
  fi
#MaxPrtyPendPacket
  if [ $id$PLACE.$i = $id$PLACE.9 ]; then
      VALUE=25  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)        
  fi
#OldestPacketAge
  if [ $id$PLACE.$i = $id$PLACE.10 ]; then
      VALUE=25  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)        
  fi
#GlobalXOff
  if [ $id$PLACE.$i = $id$PLACE.11 ]; then
      VALUE=0  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)        
  fi
#Algorithmic Time of network
  if [ $id$PLACE.$i = $id$PLACE.12 ]; then
      VALUE="NTP"  
      tmp=$(redis-cli -h $listen  SET $id$PLACE.$i $VALUE)        
  fi
done
  
exit 0
