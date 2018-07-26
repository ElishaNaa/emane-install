import os
import sys
import threading
from time import sleep
import ftplib
import pipes
import paramiko

import base64
import getpass
import socket
import traceback
import re
import time

USER = 'user'
PASSWORD = 'cisco'
SERVERS_IP_PATH = '/home/user/emane-install/destributed-platform/serverIP'
SERVERS_EMANE_TOP_DIR = '/tmp/'

port = 22

timeout = 100

PLATFORMS_TO_SERVER_FILE = "nodes2s"

def getServersAddrs(i_ServerList):
    """
    match the addresses to the servers name.
    :param i_ServerList:
    :return:
    """
    serverAddrList =[]

    with open(PLATFORMS_TO_SERVER_FILE, "r") as txtFile:
        data = txtFile.readlines()
        table = []
        filteredTable = []
        for line in data:
            if line.startswith("#"):
                continue
            eachLine = line.split(";")
            table.append(eachLine)
            filteredTable.append([])
        for element in range(0, len(table)):
            filteredTable.append(table[element][0])

    with open(SERVERS_IP_PATH) as serversFile:
        serversFileLines = serversFile.readlines()
        for line in serversFileLines:
            if line[-1:] == '\n':
                line = line[:-1]
            serverDetails = line.split(",")
            if (i_ServerList != True):
                if(serverDetails[0] in i_ServerList and serverDetails[0] in filteredTable):
                    serverAddrList.append(serverDetails[1])
            else:
                if(serverDetails[0] in filteredTable):
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

def execute_ssh_command(host, port, username, password, keyfilepath, keyfiletype, command):
    """
    execute_ssh_command(host, port, username, password, keyfilepath, keyfiletype, command) -> tuple

    Executes the supplied command by opening a SSH connection to the supplied host
    on the supplied port authenticating as the user with supplied username and supplied password or with
    the private key in a file with the supplied path.
    If a private key is used for authentication, the type of the keyfile needs to be specified as DSA or RSA.
    :rtype: tuple consisting of the output to standard out and the output to standard err as produced by the command
    """
    # setup logging
    paramiko.util.log_to_file("logging_start&stop.log")
    ssh = None
    key = None
    endtime = time.time() + timeout
    try:
        if keyfilepath is not None:
            # Get private key used to authenticate user.
            if keyfiletype == 'DSA':
                # The private key is a DSA type key.
                key = paramiko.DSSKey.from_private_key_file(keyfilepath)
            else:
                # The private key is a RSA type key.
                key = paramiko.RSAKey.from_private_key(keyfilepath)

        # Create the SSH client.
        ssh = paramiko.SSHClient()

        # Setting the missing host key policy to AutoAddPolicy will silently add any missing host keys.
        # Using WarningPolicy, a warning message will be logged if the host key is not previously known
        # but all host keys will still be accepted.
        # Finally, RejectPolicy will reject all hosts which key is not previously known.
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # Connect to the host.
        if key is not None:
            # Authenticate with a username and a private key located in a file.
            ssh.connect(host, port, username, None, key)
        else:
            # Authenticate with a username and a password.
            ssh.connect(host, port, username, password)

        # Send the command (non-blocking)
        stdin, stdout, stderr = ssh.exec_command(command)

        # Wait for the command to terminate
        while not stdout.channel.exit_status_ready() and not stdout.channel.recv_ready():
            sleep(1)

        while not stdout.channel.eof_received:
            sleep(1)
            if time.time() > endtime:
                stdout.channel.close()
                break

        stdoutstring = stdout.readlines()
        stderrstring = stderr.readlines()
        return stdoutstring, stderrstring
    finally:
        if ssh is not None:
            # Close client connection.
            ssh.close()


def doCMD(i_cmd, i_addr, senario):
    """
    uses ssh to send the start/stop commands.
    :param i_cmd: start/stop command
    :param i_addr: the addr of the server that connected to the thread
    :param senario: name of directoy senario
    :return:
    """
    command = "cd " + SERVERS_EMANE_TOP_DIR + senario + "/; echo " + PASSWORD + " | sudo -S ./" + 'demo-' + i_cmd
    (stdoutstring, stderrstring) = execute_ssh_command(i_addr, port, USER, PASSWORD, None, None, command)
    for stdoutrow in stdoutstring:
        print stdoutrow

    with open('/tmp/log-' + i_cmd, 'w+') as outfile:
        for line in stderrstring:
            outfile.write(line)

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
        if serversFileLines[-1:] == '\n':
                serversFileLines = serversFileLines[:-1]
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
    command = "test -e " + pipes.quote(path) + " && echo 0 || echo 1"
    (stdoutstring, stderrstring) = execute_ssh_command(host, port, USER, PASSWORD, None, None, command)

    for status in stdoutstring:
        if re.search('0', status):
            return True
        if re.search('1', status):
            return False

def clear (host, senario):
    command = 'cd ' + SERVERS_EMANE_TOP_DIR + '; rm -Rf ' + senario + '/'
    (stdoutstring, stderrstring) = execute_ssh_command(host, port, USER, PASSWORD, None, None, command)

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
                for host in serverAddrList:
                    if exists_remote(host, '/var/run/demo.lock') or exists_remote(host, '/var/run/demo-rt.lock'):
                        print "Run 'python simulation stop' first"
                    else:
                        clear(host, senario)
            else:
                print "Illegal command."
                usage()
        else:
             usage()
    else:
        print "Illegal number of parameters."
        usage()
