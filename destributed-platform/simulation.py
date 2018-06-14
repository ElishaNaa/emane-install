import os
import sys
import threading
from time import sleep
import paramiko
import ftplib
import subprocess
import pipes

USER = 'user'
PASSWORD = 'cisco'
SERVERS_IP_PATH = '/home/user/destributed platform/serverIP.txt'
SERVERS_EMANE_TOP_DIR = '/tmp/'

def getServersAddrs(i_ServerList):
    """
    match the addresses to the servers name.
    :param i_ServerList:
    :return:
    """
    serverAddrList =[]
    
    with open(SERVERS_IP_PATH) as serversFile:
        serversFileLines = serversFile.readlines()
        for line in serversFileLines:
            serverDetails = line.split(",")
            if (i_ServerList != True):
                if(serverDetails[0] in i_ServerList):
                    serverAddrList.append(serverDetails[1])
            else:
                serverAddrList.append(serverDetails[1])
    return serverAddrList

def run(i_cmd, i_ServerList, senario): #get servers name to run
    """
    every server requires its own thread for running the commands.
    :param i_cmd: [start/stop] (the input was checked)
           i_serverList: names of servers to execute
           senario: name of directoy senario
    :return:
    """
    threads = []
    serverAddrList = getServersAddrs(i_ServerList)
    for server in serverAddrList:
        t = threading.Thread(target=doCMD, args=(i_cmd, server, senario,))
        threads.append(t)
        t.start()

def doCMD(i_cmd, i_addr, senario):
    """
    uses ssh to send the start/stop commands. (if the command is start, addif to the emanenode0 bridge)
    //TODO: normal comunication protocol !!!
    :param i_cmd: start/stop command
    :param i_addr: the addr of the server that connected to the thread
    :param senario: name of directoy senario
    :return:
    """
    SSHCmd = "sshpass -p " + PASSWORD + " ssh -p 22 " + USER + "@" + i_addr + " "
    statopCmd = SSHCmd + "\"cd " + SERVERS_EMANE_TOP_DIR + senario + "/; echo " + PASSWORD + " | sudo -S ./" + 'demo-' + i_cmd + "\" "
    os.system(statopCmd)
    if i_cmd == 'start':
        sleep(2)
    else:
        #commands to do when stop
        pass
def checkIfServerExist(i_servers):
    """
    check if the list of servers exsit in 'serverIP.txt'
    :param i_servers: users input list of servers name
    :return: if all servers exist - empty array, if there are servers that doesn't exist - return list of their names.
    """
    inputServerNames = i_servers.split(",")
    allServersExist = inputServerNames
    serversNotExist = []
    serversNameList = []
    with open(SERVERS_IP_PATH) as serversFile:
        serversFileLines = serversFile.readlines()
        for server in serversFileLines:
            serverNames = server.split(",")
            serversNameList.append(serverNames[0])
    for server in inputServerNames:
        if server not in serversNameList:
            serversNotExist.append(server)
    if (len(serversNotExist) > 0):
        print str(serversNotExist) + " do(es) not exist."
        allServersExist = False
    return allServersExist

def exists_remote(host, path):
    """Test if a file exists at path on a host accessible with SSH."""
    status = subprocess.call(
        ['sshpass', '-p', PASSWORD, 'ssh', host, 'test -e ' + pipes.quote(path)])
    if status == 0:
        return True
    if status == 1:
        return False
    raise Exception('SSH failed')

def clear (ip, senario):
    subprocess.call(
        ['sshpass', '-p', PASSWORD, 'ssh', USER + '@' + ip, 'cd ' + SERVERS_EMANE_TOP_DIR + '; rm -Rf ' + senario + '/'])

def usage():
    print 'To start/stop the simulation, the command should be like that:'
    print 'python simulation.py start/stop SenarioNameDir (optional - servers to run, if empty; all servers will run)'
    print 'The servers need to be written like that: Name of server1,Name of server2'
    print ''
    print 'To clear the files, the command should be like that:'
    print 'python simulation.py clear SenarioNameDir (optional - servers to run, if empty; all servers will run)'
    print 'The servers need to be written like that: Name of server1,Name of server2'

if __name__ == '__main__':
    numbOfSysArg = len(sys.argv)
    if numbOfSysArg == 3 or numbOfSysArg == 4:
        cmd = sys.argv[1]
        senario = sys.argv[2]
        if numbOfSysArg == 4:
            serverNames = sys.argv[3]
            serversExist = checkIfServerExist(serverNames)
        else:
            serversExist = True
        if(serversExist != False):
            if cmd == 'start' or cmd == 'stop':
                if numbOfSysArg == 4:
                    run(cmd, serverNames, senario)
                else:
                    run(cmd, serversExist, senario)
            elif cmd == 'clear':
                if numbOfSysArg == 4:
                     serverAddrList = getServersAddrs(serverNames)
                else:
                    serverAddrList = getServersAddrs(serversExist)
                for ip in serverAddrList:
                    if exists_remote(USER + '@' + ip, '/var/run/demo.lock') or exists_remote(USER + '@' + ip, '/var/run/demo-rt.lock'):
                        print "Run 'python simulation stop' first"
                    else:
                        clear(ip, senario)
            else:
                print "Illegal command."
                usage()
        else:
             usage()
    else:
        print "Illegal number of parameters."
        usage()