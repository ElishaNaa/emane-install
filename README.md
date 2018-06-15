# emane-install

### Prerequisites

On all the machines you need to install this dependency:
```
ftp
ftplib-dev
sshpass
redis-server
bridge-utils
openvswitch-switch
openvswitch-controller
```
On machines that you want to distribute the files you need also to install ``` ftpd  ```

##### Remind you need to allow port 22

Additionally on all the machines you need to install docker like this:
```
$ sudo apt-get install \
    apt-transport-https \
    ca-certificates \
    curl \
    software-properties-common
curl -fsSL https://download.docker.com/linux/ubuntu/gpg | sudo apt-key add -
$ sudo add-apt-repository \
   "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
   $(lsb_release -cs) \
   stable"

$ sudo apt-get update
$ sudo apt-get install docker-ce=17.09.0~ce-0~ubuntu
```
After the installion you need to destrbuite the docker image, so you should build the docker image on one machine like this:
```
$ sudo docker build -f dockerFileForModem -t fullsimdoc .

$ sudo docker save -o fullsimdocTar fullsimdoc
```
And distribute ``` fullsimdocTar  ``` to all machine

After this extricate the image
```
$ sudo docker load -i fullsimdocTar
```

You need to make sure that the machines are synchronized, so you have to choose between machines a machine that will be an ntp server on which ntp is installed.

On the other machines we will also install ntp and we will also write the following command:
```
$ sudo uptitude install ntpt
```
The file /etc/ntp.conf on server should contain the following lines:
```
server 127.127.1.0
fudge 127.127.1.0 stratum 10 
```
The file /etc/ntp.conf on other machines should contain the following line:
```
server X.X.X.X
```
##### Remind you need to allow port 123

**********************************************************

### Test

```
$ cd destributed-platform
```
There are 3 files that need to be prepared in advance, in which you will specify the details and method of distributing the containers.

1) nodes2s - in each row you define the name of the machine and the nodeID you want to run on it (Emane02; 1:2 -----> A machine called Emane02 will run the containers with IDs 1 and 2. It is important to keep the name The nodeID will be the character ``` ; ``` ).

2) In each line you define the name of the machine and its IP (eg Emane02,X.X.X.X It is important to keep the name of the machine to the IP, the character ``` , ```).

3) dockerImage - In each line you define the images of the modem and the router running on the machine (eg modem=fullsimdoc,1=fullsimdoc,2=fullsimdoc).

To create the number of node (s) you want to run in senario, write the following command:

```
$ sudo python configureServer.py create 3 4
(so that 3 is the name of the scenario)
```

To distribute the scenario to the files defined in the files that you have been asked to prepare in advance, you should type the following command:

```
$ sudo python configureServer.py spread 3 
(so that 3 is the name of the scenario)
```
To start the scenario write the following command:

```
$ sudo python simulation.py start 3  
(so that 3 is the name of the scenario)
```
To finish running the scenario, write the following command:

```
$ sudo python simulation.py stop 3  
(so that 3 is the name of the scenario)
```
To clear the scenario from the machines, write the following command:

```
$ sudo python simulation.py clear 3  
(so that 3 is the name of the scenario)
```

If you do not want to run / stop / clear all the machines that you defined in the nodes2s file, you can write a command from the following configuration:

```
$ sudo python simulation.py start/stop/clear 3 Emane02,Emane04
(so that 3 is the name of the scenario and Emane02 and Emane04 are the names of the machines you want to run and defined in the files you were asked to prepare above).
```

#### If you want to change the scenario information, you need to access the destributed-platform/template folder

