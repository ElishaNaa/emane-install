import ftplib
import re
import sys
import os
from os import listdir
from os.path import isfile, join
import shutil, errno
import re
import subprocess
import pipes
import paramiko

import base64
import getpass
import socket
import traceback

from copy import deepcopy

NODES_DIRECTORY_PATH_SERVER = '/tmp/'  # work with Emane Servers

USER_NAME_TO_SERVERS = 'user'
PASSWORD_TO_SERVERS = 'cisco'
PLATFORMS_TO_SERVER_FILE = "nodes2s"
ADDRESS_OF_SERVERS = "serverIP"

EMANE_TEMPLATES_PATH_HOST = os.path.dirname(os.path.realpath(__file__)) + '/templates/'
EMANE_HOST_PATH = '/tmp/'

ServerIPNodeID = 'ServerIPNodeID'
dockerImage = 'dockerImage'

port = 22


class distribToServers():
    def __init__(self):
        self.txtFile = open(PLATFORMS_TO_SERVER_FILE, "r")

    def connect(self, hostname):
        # setup logging
        paramiko.util.log_to_file("logging_destrib.log")
        # Paramiko client configuration
        UseGSSAPI = True  # enable GSS-API / SSPI authentication
        DoGSSAPIKeyExchange = True
        # get host key, if we know one
        hostkey = None
        try:
            host_keys = paramiko.util.load_host_keys(
                os.path.expanduser("~/.ssh/known_hosts")
            )
        except IOError:
            try:
                # try ~/ssh/ too, because windows can't have a folder named ~/.ssh/
                host_keys = paramiko.util.load_host_keys(
                    os.path.expanduser("~/ssh/known_hosts")
                )
            except IOError:
                print("*** Unable to open host keys file")
                host_keys = {}
        if hostname in host_keys:
            hostkeytype = host_keys[hostname].keys()[0]
            hostkey = host_keys[hostname][hostkeytype]
            print("Using host key of type %s" % hostkeytype)
        try:
            t = paramiko.Transport((hostname, port))
            t.connect(
                hostkey,
                USER_NAME_TO_SERVERS,
                PASSWORD_TO_SERVERS,
                gss_host=socket.getfqdn(hostname),
            )
        except Exception as e:
            print("*** Caught exception: %s: %s" % (e.__class__, e))
            traceback.print_exc()
            try:
                t.close()
            except:
                pass
            sys.exit(1)
        return t

    def readFile(self):
        """
        converts the input lines to list of lists;
        on every list, the first element is the server name, the second element is list of nodes id.
        on 'readFile' it separates the server name from the id's.
        then sends the id's to 'scanNumbers' to get a regular list of numbers.
        :return: list of lists; sever name with their nodes id.
        """
        data = self.txtFile.readlines()
        table = []
        filteredTable = []
        for line in data:
            if line.startswith("#"):
                continue
            eachLine = line.split(";")
            table.append(eachLine)
            filteredTable.append([])
        for element in range(0, len(table)):
            filteredTable[element].append(table[element][0])
            filteredTable[element].append(self.scanNumbers(table[element][1]))
        return filteredTable

    def scanNumbers(self, i_NumbersString):
        '''
        convert the list of the numbers to normal list of numbers.
        :param i_NumbersString:
        :return: digitsArrey- arrey of numbers
        '''
        digitsArrey = []
        platNames = i_NumbersString.split(",")
        for index in range(0, len(platNames)):
            testName = platNames[index]
            if testName[-1:] == '\n':
                testName = testName[:-1]
            if ":" in testName:
                testRange = testName.split(":")
                for j in range(int(testRange[0]), int(testRange[1]) + 1):
                    if j not in digitsArrey:
                        digitsArrey.append(j)
            else:
                if testName not in digitsArrey:
                    digitsArrey.append(testName)
        return digitsArrey

    def distribServersIP(self, i_IpPath, i_TransmissionType):
        id = 1
        for server in i_IpPath:
            serverAddr = server[0]
            if serverAddr[-1:] == '\n':
                serverAddr = serverAddr[:-1]
            content = 'hostID=' + str(id) + '\n'
            content += serverAddr
            for i in server[1]:
                content += ',' + str(i)
            for IP in i_IpPath:
                if IP[0][-1:] == '\n':
                    IP[0] = IP[0][:-1]
                if serverAddr != IP[0]:
                    content += '\n' + str(IP[0])

            with open(EMANE_HOST_PATH + ServerIPNodeID, 'w+') as outfile:
                outfile.write(content)

            transport = self.connect(serverAddr)

            sftp = paramiko.SFTPClient.from_transport(transport)
            sftp.chdir(NODES_DIRECTORY_PATH_SERVER + i_TransmissionType)
            sftp.put(EMANE_HOST_PATH + ServerIPNodeID,
                     NODES_DIRECTORY_PATH_SERVER + i_TransmissionType + '/' + ServerIPNodeID)  # copy all files from host to server
            sftp.chmod(NODES_DIRECTORY_PATH_SERVER + i_TransmissionType + '/' + ServerIPNodeID, 0777)  # chmod the files
            os.remove(EMANE_HOST_PATH + ServerIPNodeID)
            id += 1

    def matchID(self, i_nodes, imgID):
        ipAddrImageId = i_nodes
        for line in ipAddrImageId:
            ID = line[1]
            for image in imgID:
                if image[0] == ID:
                    imgFound = image[1]
                    pass
            line[1] = imgFound
            imgFound = None
        return ipAddrImageId

    def distribServersImages(self, i_IpPath, i_TransmissionType):
        with open(dockerImage, 'r') as dockerIm:
            images = dockerIm.readlines()
        nodeID = []
        for line in images:
            numbers = re.findall('[0-9]+=', line)
            result = []

            if not numbers:
                continue
            else:
                for number in numbers:
                    numArry = number.split("=")
                    result.append(int(numArry[0]))
                nodeID.append([result, line])
        i_IpPath = self.matchID(i_IpPath, nodeID)
        for server in i_IpPath:
            serverAddr = server[0]
            content = server[1]
            if content == None:
                print 'something happen, check please the files ''nodes2s'', ''serverIP'' and ''dockerimage'''
                for ip in i_IpPath:
                    serverAddr = ip[0]
                    if serverAddr != None:
                        subprocess.call(
                            ['sshpass', '-p', PASSWORD_TO_SERVERS, 'ssh', USER_NAME_TO_SERVERS + '@' + serverAddr,
                             'cd ' + NODES_DIRECTORY_PATH_SERVER + '; rm -Rf ' + i_TransmissionType + '/'])
                return
            with open(EMANE_HOST_PATH + 'Images', 'w+') as outfile:
                outfile.write(content)

            transport = self.connect(serverAddr)

            sftp = paramiko.SFTPClient.from_transport(transport)
            sftp.chdir(NODES_DIRECTORY_PATH_SERVER + i_TransmissionType)
            sftp.put(EMANE_HOST_PATH + 'Images',
                     NODES_DIRECTORY_PATH_SERVER + i_TransmissionType + '/' + dockerImage)  # copy all files from host to server
            sftp.chmod(NODES_DIRECTORY_PATH_SERVER + i_TransmissionType + '/' + dockerImage, 0777)  # chmod the files
            os.remove(EMANE_HOST_PATH + 'Images')

    '''      
    def getPlatformPath(self, i_ServerPlatformArrey, i_Suffix):
        """
        add to the node number, all the path
        :param i_ServerPlatformArrey:
        :param i_Suffix:
        :return:
        """
        platformsPath = i_ServerPlatformArrey
        for server in range(0, len(platformsPath)):
            for platform in range(0, len(platformsPath[server][1])):
                numberOfPlatform = str(platformsPath[server][1][platform])
                path = nodesDirPathHost + '/' + i_Suffix + numberOfPlatform + '.xml'
                platformsPath[server][1][platform] = path
        return platformsPath
    '''

    def matchAddresses(self, i_nodes):
        """
        open the file with the addresses of the servers, and replace between them.
        :param i_nodes: list of nodes with the server name
        :return: list of nodes with the server addr
        """
        ipAddrPlatformsPath = i_nodes
        addrArrey = []
        addrFound = None
        with open(ADDRESS_OF_SERVERS, 'r') as addressFile:
            dataAddr = addressFile.readlines()
            for line in dataAddr:
                # if line.startswith("#"):
                #   continue
                eachLine = line.split(",")
                addrArrey.append([eachLine[0], eachLine[1]])
        for line in ipAddrPlatformsPath:
            nameOfServer = line[0]
            for addr in addrArrey:
                if addr[0] == nameOfServer:
                    addrFound = addr[1]
                    pass
            line[0] = addrFound
            addrFound = None
        return ipAddrPlatformsPath

    def distribPlatforms(self, i_IpPath, i_TransmissionType):
        '''
        The method distrib all the folder from the host to all the servers.
        Then on every server deletes the files that should not be there (only by the node ID).
        :param i_IpPath:
        :return:
        '''
        for server in i_IpPath:

            nodesDirPathHost = EMANE_HOST_PATH + 'scenario-' + i_TransmissionType + '/'
            nodesDirPathServer = NODES_DIRECTORY_PATH_SERVER + i_TransmissionType + '/'

            nodeID = []
            serverAddr = server[0]
            transport = self.connect(serverAddr)
            sftp = paramiko.SFTPClient.from_transport(transport)
            sftp.chdir(NODES_DIRECTORY_PATH_SERVER)
            sftp.mkdir(i_TransmissionType)
            sftp.chmod(nodesDirPathServer, 0777)  # chahnge mode of this folder to 777
            sftp.chdir(nodesDirPathServer)
            hostFiles = [f for f in listdir(nodesDirPathHost) if isfile(join(nodesDirPathHost, f))]
            for fileName in hostFiles:
                file = open(nodesDirPathHost + fileName)
                sftp.put(nodesDirPathHost + fileName,
                         nodesDirPathServer + fileName)  # copy all files from host to server
                sftp.chmod(nodesDirPathServer + fileName, 0777)  # chmod the files
                file.close()
            listfile = ['tdmactmac.xml', 'NO-host-emaneeventservice', 'node-prestop', 'redis.EXAMPLE', 'demo-start', 'demo-stop',
                        'protocol-config.xsd', 'emanelayerdlep.xml', 'emanelayersnmp.xml', 'emanelayerfilter.xml',
                        'eventservice.xml', 'eventdaemon.xml', 'transraw.xml', 'tdmamac.xml', 'credit-windowing-03.xml',
                        'dlep-draft-24.xml', 'dlep-rfc-8175.xml', 'schedule.xml']
            flag = 0
            for i in server[1]:  # make list with all the nodes ID's
                n = re.findall('\d+', str(i))[-1]
                nodeID.append(int(n))
            namesAllFiles = sftp.listdir(".")
            for testFile in namesAllFiles:  # in the server, nodes that not in the list of the nodes ID, will be deleted.
                fn = re.findall('\d+', str(testFile))
                if fn:
                    fn = int(fn[-1])
                    if fn not in nodeID:
                        for file in listfile:
                            if testFile == file:
                                flag = 1
                        if flag == 0:
                            sftp.remove(nodesDirPathServer + testFile)
                flag = 0

            sftp.chdir(nodesDirPathServer)
            sftp.mkdir('scripts')  # make folder
            sftp.chmod(nodesDirPathServer + '/scripts/', 0777)  # chahnge mode of this folder to 777
            sftp.chdir(nodesDirPathServer + '/scripts/')  # enter to this folder
            nodesDirPathHost = EMANE_HOST_PATH + 'scenario-' + i_TransmissionType + '/scripts/'
            hostFiles = [f for f in listdir(nodesDirPathHost) if isfile(join(nodesDirPathHost, f))]

            # list all files that in host's folder
            for fileName in hostFiles:
                file = open(nodesDirPathHost + fileName)
                sftp.put(nodesDirPathHost + fileName,
                         nodesDirPathServer + '/scripts/' + fileName)  # copy all files from host to server
                sftp.chmod(nodesDirPathServer + '/scripts/' + fileName, 0777)  # chmod the files
                file.close()

            transport.close()

    def isdir(self, ftp, name):
        try:
            ftp.cwd(name)
            ftp.cwd('..')
            return True
        except:
            return False

    def ftpRemoveTree(self, i_FTP, i_Dir):
        '''
        Delete recuresvly all folders anf files.
        :param i_FTP:
        :param i_Dir: the father folder
        :return:
        '''
        wd = i_FTP.pwd()
        try:
            names = i_FTP.nlst(i_Dir)
        except ftplib.all_errors as e:
            # FTP Server error
            dirExists = self.isdir(i_FTP, NODES_DIRECTORY_PATH_SERVER + i_Dir)
            if (dirExists == True):
                i_FTP.rmd(i_Dir)
            return
        for name in names:
            if os.path.split(name) in ('.', '..'): continue
            try:
                i_FTP.cwd(name)
                i_FTP.cwd(wd)
                self.ftpRemoveTree(i_FTP, name)
            except ftplib.all_errors:
                i_FTP.delete(name)
        try:
            i_FTP.rmd(i_Dir)
        except ftplib.all_errors as e:
            # FTP Server error
            return
        finally:
            None


class hatchNodes():
    def __init__(self, i_TransmissionType, i_NumberOfNodes, i_Base):
        self.transmissionType = i_TransmissionType
        self.numberOfNodes = i_NumberOfNodes
        self.i_Base = i_Base
        self.txtFile = open(PLATFORMS_TO_SERVER_FILE, "r")
        self.StartHatching(self.transmissionType, self.numberOfNodes, i_Base)

    def StartHatching(self, i_TransmiionType, i_NumberOfNodes, i_Base):
        self.hatchRouting(i_TransmiionType, i_NumberOfNodes)
        self.hatchTDMAPlatform(i_TransmiionType, i_NumberOfNodes)
        #self.hatchTDMANEM(i_TransmiionType, i_NumberOfNodes)
        if 'tdmact' in i_TransmiionType:
             self.hatchTDMACTNem(i_TransmiionType, i_NumberOfNodes)
        else:
            self.hatchTDMANEM(i_TransmiionType, i_NumberOfNodes)
        self.hatchFileAlways(i_TransmiionType, i_NumberOfNodes)
        self.hatchScripts(i_TransmiionType)
        self.hatchRouters(i_TransmiionType, i_NumberOfNodes, i_Base)
        self.hatchOtestPoint(i_TransmiionType, i_NumberOfNodes)
        # self.copyanything(EMANE_TEMPLATES_PATH_HOST + 'scripts', EMANE_HOST_PATH + i_TransmiionType + '/scripts')

    def scanNumbers(self, i_NumbersString):
        '''
        convert the list of the numbers to normal list of numbers.
        :param i_NumbersString:
        :return: digitsArrey- arrey of numbers
        '''
        digitsArrey = []
        platNames = i_NumbersString.split(",")
        for index in range(0, len(platNames)):
            testName = platNames[index]
            if testName[-1:] == '\n':
                testName = testName[:-1]
            if ":" in testName:
                testRange = testName.split(":")
                for j in range(int(testRange[0]), int(testRange[1]) + 1):
                    if j not in digitsArrey:
                        digitsArrey.append(j)
            else:
                if testName not in digitsArrey:
                    digitsArrey.append(testName)
        return digitsArrey

    def readFile(self):
        """
        converts the input lines to list of lists;
        on every list, the first element is the server name, the second element is list of nodes id.
        on 'readFile' it separates the server name from the id's.
        then sends the id's to 'scanNumbers' to get a regular list of numbers.
        :return: list of lists; sever name with their nodes id.
        """
        data = self.txtFile.readlines()
        table = []
        filteredTable = []
        for line in data:
            if line.startswith("#"):
                continue
            eachLine = line.split(";")
            table.append(eachLine)
            filteredTable.append([])
        for element in range(0, len(table)):
            filteredTable[element].append(table[element][0])
            filteredTable[element].append(self.scanNumbers(table[element][1]))

        ipAddrPlatformsPath = filteredTable
        addrArrey = []
        addrFound = None
        with open(ADDRESS_OF_SERVERS, 'r') as addressFile:
            dataAddr = addressFile.readlines()
            for line in dataAddr:
                # if line.startswith("#"):
                #   continue
                eachLine = line.split(",")
                addrArrey.append([eachLine[0], eachLine[1]])
        for line in ipAddrPlatformsPath:
            nameOfServer = line[0]
            for addr in addrArrey:
                if addr[0] == nameOfServer:
                    addrFound = addr[1]
                    pass
            line[0] = addrFound
            addrFound = None
        return ipAddrPlatformsPath

    def matchID(self, i_nodes, imgID):
        ipAddrImageId = []
        ipAddrImageId = deepcopy(i_nodes)
        for line in ipAddrImageId:
            ID = line[1]
            for image in imgID:
                if image[0] == ID:
                    imgFound = image[1]
                    pass
            line[1] = imgFound
            imgFound = None
        return ipAddrImageId

    def hatchRouters(self, i_TransmiionType, i_NumberOfNodes, i_Base):
        IpPath = []
        pathWhereFind = EMANE_HOST_PATH + 'scenario-' + i_TransmiionType + '/'

        if i_Base == None:
            i_Base = 0
        else:
            i_Base = int(i_Base)

        self.removeFile(pathWhereFind, 'RA')
        addrPath = self.readFile()

        with open(dockerImage, 'r') as dockerIm:
            images = dockerIm.readlines()
        nodeID = []
        for line in images:
            numbers = re.findall('[0-9]+=', line)
            result = []

            if not numbers:
                continue
            else:
                for number in numbers:
                    numArry = number.split("=")
                    result.append(int(numArry[0]))
                nodeID.append([result, line])
        IpPath = self.matchID(addrPath, nodeID)

        for index in range(1, i_NumberOfNodes):
            rid = i_Base + 1
            flag = 0
            for addr in addrPath:
                for ID in addr[1]:
                    if index == ID:
                        ip = addr[0]
                        if ip[-1:] == '\n':
                            ip = ip[:-1]
                        for element in IpPath:
                            temp = element[1].split(',')
                            for line in temp:
                                if "rt-" + str(index) + "=" in line:
                                    tmp = line.split("rt-" + str(index) + "=")
                                    image = tmp[1]
                                    if image[-2:] == '\r\n':
                                        image = image[:-2]
                                    if image[-1:] == '\n':
                                        image = image[:-1]

                                    replacements = [["NEMID", str(index)], ["NETTYPE", '1'], ["RID", str(rid)],
                                                    ["SERVERIP", str(ip)], ["IMAGE", str(image)]]
                                    template = EMANE_TEMPLATES_PATH_HOST + 'RA.json.template'
                                    pathToNewFiles = pathWhereFind + 'RA' + str(index) + '.json'
                                    self.preprocess(replacements, template, pathToNewFiles)
                                    i_Base = i_Base + 1
                                    flag = 1
                                    break
                            if flag == 1:
                                break
                    if flag == 1:
                        break
                if flag == 1:
                    break


    def hatchScripts(self, i_TransmiionType):
        listfile = ['docker-democtl-host', 'docker-demo-init', 'docker-rtctl-host', 'docker-rtdemo-init', 'run-snmpd.sh',
                    'snmpflushallDB.sh', 'snmpsetDB.sh', 'readJson.py']
        pathWhereFind = EMANE_HOST_PATH + 'scenario-' + i_TransmiionType + '/scripts/'
        exists = os.path.exists(pathWhereFind)
        if exists:
            shutil.rmtree(pathWhereFind)
        os.makedirs(pathWhereFind)
        replacements = []
        for file in listfile:
            template = EMANE_TEMPLATES_PATH_HOST + 'scripts/' + file + '.template'
            pathToNewFiles = pathWhereFind + file
            self.preprocess(replacements, template, pathToNewFiles)


    def hatchFileAlways(self, i_TransmiionType, i_NumberOfNodes):
        listfile = ['eventservice', 'eelgenerator', 'emanelayerdlep', 'emanelayersnmp', 'emanelayerfilter', 'eventservice',
                    'eventdaemon', 'transraw', 'tdmamac', 'credit-windowing-03', 'dlep-draft-24', 'dlep-rfc-8175',
                    'schedule']
        pathWhereFind = EMANE_HOST_PATH + 'scenario-' + i_TransmiionType + '/'
        replacements = []
        if 'tdmact' in i_TransmiionType:
            listfile.remove('tdmamac')

        for file in listfile:
            template = EMANE_TEMPLATES_PATH_HOST + file + '.xml.template'
            pathToNewFiles = pathWhereFind + file + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)
        template = EMANE_TEMPLATES_PATH_HOST + 'protocol-config.xsd.template'
        pathToNewFiles = pathWhereFind + 'protocol-config.xsd'
        self.preprocess(replacements, template, pathToNewFiles)
        if 'tdmact' in i_TransmiionType:
            demoNem = 'tdmactnem'
        else:
            demoNem = 'tdmanem'
        replacements = [["DEMOID", i_TransmiionType], ["DEMOIDNEM", demoNem]]
        template = EMANE_TEMPLATES_PATH_HOST + 'demo-start.template'
        pathToNewFiles = pathWhereFind + 'demo-start'
        self.preprocess(replacements, template, pathToNewFiles)
        replacements = [["DEMOID", i_TransmiionType]]
        template = EMANE_TEMPLATES_PATH_HOST + 'demo-stop.template'
        pathToNewFiles = pathWhereFind + 'demo-stop'
        self.preprocess(replacements, template, pathToNewFiles)
        replacements = []
        template = EMANE_TEMPLATES_PATH_HOST + 'redis.EXAMPLE.template'
        pathToNewFiles = pathWhereFind + 'redis.EXAMPLE'
        self.preprocess(replacements, template, pathToNewFiles)
        replacements = []
        template = EMANE_TEMPLATES_PATH_HOST + 'scenario.eel.template'
        pathToNewFiles = pathWhereFind + 'scenario.eel'
        self.preprocess(replacements, template, pathToNewFiles)
        replacements = []
        template = EMANE_TEMPLATES_PATH_HOST + 'node-prestop.template'
        pathToNewFiles = pathWhereFind + 'node-prestop'
        self.preprocess(replacements, template, pathToNewFiles)
        replacements = []
        template = EMANE_TEMPLATES_PATH_HOST + 'NO-host-emaneeventservice.template'
        pathToNewFiles = pathWhereFind + 'NO-host-emaneeventservice'
        self.preprocess(replacements, template, pathToNewFiles)
        if 'tdmact' in i_TransmiionType:
            for index in range(1, i_NumberOfNodes):
            
                logPath = NODES_DIRECTORY_PATH_SERVER + i_TransmiionType + '/persist/modem-' + str(index) + '/var/log/CT'
                replacements = [["LOG", logPath]]
                template = EMANE_TEMPLATES_PATH_HOST + 'tdmactmac.xml.template'
                pathToNewFiles = pathWhereFind + 'tdmactmac' + str(index) + '.xml'
                self.preprocess(replacements, template, pathToNewFiles)


    def hatchTDMAPlatform(self, i_TransmissionType, i_NumberOfNodes):
        pathWhereFind = EMANE_HOST_PATH + 'scenario-' + i_TransmissionType + '/'
        self.removeFile(pathWhereFind, 'platform')
        for index in range(1, i_NumberOfNodes):
            if 'tdmact' in i_TransmissionType:
                nemxmlChangeTo = 'tdmactnem' + str(index) + '.xml'
            else:
                nemxmlChangeTo = 'tdmanem' + str(index) + '.xml'

            replacements = [["NEMID", str(index)], ["NEMXML", nemxmlChangeTo]]
            template = EMANE_TEMPLATES_PATH_HOST + 'platform.xml.template'
            pathToNewFiles = pathWhereFind + 'platform' + str(index) + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)


    def hatchOtestPoint(self, i_TransmissionType, i_NumberOfNodes):
        pathWhereFind = EMANE_HOST_PATH + 'scenario-' + i_TransmissionType + '/'
        self.removeFile(pathWhereFind, 'otestpoint')
        for index in range(1, i_NumberOfNodes):
            replacements = [["NEMID", str(index)]]
            template = EMANE_TEMPLATES_PATH_HOST + 'otestpointd.xml.template'
            pathToNewFiles = pathWhereFind + 'otestpointd' + str(index) + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)
            replacements = [["NEMID", str(index)]]
            template = EMANE_TEMPLATES_PATH_HOST + 'otestpoint-recorder.xml.template'
            pathToNewFiles = pathWhereFind + 'otestpoint-recorder' + str(index) + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)
            replacements = [["NEMID", str(index)]]
            template = EMANE_TEMPLATES_PATH_HOST + 'otestpoint-broker.xml.template'
            pathToNewFiles = pathWhereFind + 'otestpoint-broker' + str(index) + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)
        listfile = ['probe-emane-physicallayer', 'probe-emane-rawtransport', 'probe-emane-tdmaeventschedulerradiomodel']
        replacements = []
        for file in listfile:
            template = EMANE_TEMPLATES_PATH_HOST + file + '.xml.template'
            pathToNewFiles = pathWhereFind + file + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)


    def hatchTDMANEM(self, i_TransmissionType, i_NumberOfNodes):
        pathWhereFind = EMANE_HOST_PATH + 'scenario-' + i_TransmissionType + '/'
        self.removeFile(pathWhereFind, 'tdmanem')
        for index in range(1, i_NumberOfNodes):
            hexa = '{:X}'.format(index)
            if index <= 15:
                replacements = [["NEMID", str(index)], ["HEX", '0' + str(hexa)]]
            else:
                replacements = [["NEMID", str(index)], ["HEX", str(hexa)]]
            template = EMANE_TEMPLATES_PATH_HOST + 'tdmanem.xml.template'
            pathToNewFiles = pathWhereFind + 'tdmanem' + str(index) + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)


    def hatchTDMACTNem(self, i_TransmissionType, i_NumberOfNodes):
        pathWhereFind = EMANE_HOST_PATH + 'scenario-' + i_TransmissionType + '/'
        self.removeFile(pathWhereFind, 'tdmactnem')
        for index in range(1, i_NumberOfNodes):
            hexa = '{:X}'.format(index)
            if index <= 15:
                replacements = [["NEMID", str(index)], ["HEX", '0' + str(hexa)]]
            else:
                replacements = [["NEMID", str(index)], ["HEX", str(hexa)]]
            template = EMANE_TEMPLATES_PATH_HOST + 'tdmactnem.xml.template'
            pathToNewFiles = pathWhereFind + 'tdmactnem' + str(index) + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)


    def hatchRouting(self, i_TransmissionType, i_NumberOfNodes):
        fileName = 'routing'
        pathWhereFind = EMANE_HOST_PATH + 'scenario-' + i_TransmissionType + '/'
        self.removeFile(pathWhereFind, fileName)
        for index in range(1, i_NumberOfNodes):
            replacements = [["NODEID", str(index)], ["DEMOID", i_TransmissionType]]
            template = EMANE_TEMPLATES_PATH_HOST + fileName + '.conf.template'
            i_PathToNewFiles = pathWhereFind + fileName + str(index) + '.conf'
            self.preprocess(replacements, template, i_PathToNewFiles)


    def preprocess(self, replacements, template, newfile):
        with open(template, 'r') as infile:
            content = infile.read()
        for sub in replacements:
            content = content.replace('@' + sub[0] + '@', str(sub[1]))
        with open(newfile, 'w') as outfile:
            outfile.write(content)


    def removeFile(self, i_Path, i_NameOfFile):
        exists = os.path.exists(i_Path)
        if exists:
            for file in os.listdir(i_Path):
                if file.startswith(i_NameOfFile):
                    file = i_Path + '/' + file
                    os.remove(file)
        else:
            os.makedirs(i_Path)


'''
def copyanything(self, src, dst):
    try:
        shutil.copytree(src, dst)
    except OSError as exc: # python >2.5
        if exc.errno == errno.ENOTDIR:
            shutil.copy(src, dst)
        else: raise
'''


def usage():
    print'To create nodes run: '
    print'   python configureServer.py create [tdma-1/tdma] number_of_nodes (optional - base)'
    print'To distrib the nodes to the servers:'
    print'   First, write correct ''nodes2s.txt'' file, then run:'
    print'   python configureServer.py distrib [tdma-1/tdma (which nodes to distrib)]'


if __name__ == '__main__':
    numbOfSysArg = len(sys.argv)
    if len(sys.argv) == 3 or len(sys.argv) == 4 or len(sys.argv) == 5:
        if sys.argv[1] == 'create':
            transmissionType = sys.argv[2]
            numberOfNodes = sys.argv[3]
            base = None
            if numbOfSysArg == 5:
                base = sys.argv[4]
            hatchNodes(transmissionType, int(numberOfNodes) + 1, base)
        elif sys.argv[1] == 'distrib':
            transmissionType = sys.argv[2]
            work = distribToServers()
            listSrvNodes = work.readFile()
            # listPath = work.getPlatformPath(listSrvPlat)
            addrPath = work.matchAddresses(listSrvNodes)

            work.distribPlatforms(addrPath, transmissionType)
            work.distribServersIP(addrPath, transmissionType)
            work.distribServersImages(addrPath, transmissionType)

            work.txtFile.close()
        else:
            print "Illegal command."
            usage()
    else:
        print "Illegal number of parameters."
        usage()
