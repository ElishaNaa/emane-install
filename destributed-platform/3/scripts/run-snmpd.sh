#!/bin/bash

PREFIX=$(pwd | awk -F '/emane-dlep-tutorial/3' '{print $1}')

cp /etc/snmp/snmpd.EXAMPLE /etc/snmp/snmpd$1.conf

cp $PREFIX/SNMP-gen/net-snmp-5.7.3/local/passtest.EXAMPLE $PREFIX/SNMP-gen/net-snmp-5.7.3/local/passtest$1.sh

MyFile=/etc/snmp/snmpd$1.conf


newline="agentAddress  udp:10.100.$1.254:161"
sed -i -e 's/#######@agentAddress listen!@########/'"${newline}/" ${MyFile}

newline="pass .1.3.6.1.4.1.16215.1.24.1.4  "
sed -i -e 's/#here pass & mib/'"${newline}/" ${MyFile}

PREFIXB=$(printf "%s" "$PREFIX" | sed 's/\//\\\//g')
sed -i -e 's/@PREFIX@/'"${PREFIXB}/" ${MyFile}

newline="$1"
sed -i -e 's/@ID@/'"${newline}/g" ${MyFile}
 

ipAddressRedis="$2"
sed -i -e 's/@flag for ipAddressRedis@/'"${ipAddressRedis}/" $PREFIX/SNMP-gen/net-snmp-5.7.3/local/passtest$1.sh

/sbin/iptables -A OUTPUT -p udp -d 10.100.$1.254 --dport 161 -j ACCEPT
/sbin/iptables -A OUTPUT -p udp --dport 161 -j DROP

sudo snmpd -f -Lf $PREFIX/emane-dlep-tutorial/3/persist/modem-$1/var/log/snmpd.log -Ducd-snmp/pass -C -c /etc/snmp/snmpd$1.conf  &


sudo snmptrapd -f -Lf $PREFIX/emane-dlep-tutorial/3/persist/modem-$1/var/log/snmptrapd.log -Ducd-snmp/pass -C -c /etc/snmp/snmptrapd.conf  &


exit 0
