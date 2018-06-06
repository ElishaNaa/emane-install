#!/bin/bash -


#PLANE
echo "start MGEN on 7"
(ssh 10.99.0.7 'cd Desktop/emane-tutorial/23aliakim/ && mgen input mgen7 output /home/ubuntu/Desktop/emane-tutorial/23aliakim/persist/7/var/log/mgen.out txlog &> /home/ubuntu/Desktop/emane-tutorial/23aliakim/persist/7/var/run/mgen.pid') &

#GROUND
echo "start MGEN on 1"
(ssh 10.99.0.1 'cd Desktop/emane-tutorial/23aliakim/ && mgen input mgen1 output /home/ubuntu/Desktop/emane-tutorial/23aliakim/persist/1/var/log/mgen.out') &
echo "start MGEN on 2"
(ssh 10.99.0.2 'cd Desktop/emane-tutorial/23aliakim/ && mgen input mgen2 output /home/ubuntu/Desktop/emane-tutorial/23aliakim/persist/2/var/log/mgen.out') &
echo "start MGEN on 3"
(ssh 10.99.0.3 'cd Desktop/emane-tutorial/23aliakim/ && mgen input mgen3 output /home/ubuntu/Desktop/emane-tutorial/23aliakim/persist/3/var/log/mgen.out') &

sleep 2

./mgenstat
