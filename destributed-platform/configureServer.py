import ftplib
import re
import sys
import os
from os import listdir
from os.path import isfile, join

NODES_DIRECTORY_PATH_HOST_TDMA = '/tmp/3/'
NODES_DIRECTORY_PATH_SERVER = '/tmp/' #work with Emane Servers

USER_NAME_TO_SERVERS = 'user'
PASSWORD_TO_SERVERS = 'cisco'
PLATFORMS_TO_SERVER_FILE = "nodes2s.txt"
ADDRESS_OF_SERVERS = "serverIP.txt"

EMANE_TEMPLATES_PATH_HOST = '/tmp/templates/'
EMANE_HOST_PATH = '/tmp/'

ServerIPNodeID = 'ServerIPNodeID'

class spreadToServers():
    def __init__(self):
        self.txtFile = open(PLATFORMS_TO_SERVER_FILE, "r")

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
                for j in range(int(testRange[0]), int(testRange[1])+1):
                    if j not in digitsArrey:
                        digitsArrey.append(j)
            else:
                if testName not in digitsArrey:
                    digitsArrey.append(testName)
        return digitsArrey

    def spreadServersIP(self, i_IpPath, i_TransmissionType):
        id = 1
        for server in i_IpPath:
            serverAddr = server[0]
            content = 'hostID=' + str(id) + '\n'
            content += serverAddr
            for i in server[1]:
                content += ',' + str(i)
            for IP in i_IpPath:
            	if serverAddr != IP[0]:
            		content += '\n' + str(IP[0])
            
            with open(EMANE_HOST_PATH + ServerIPNodeID, 'w+') as outfile:
                outfile.write(content)

            session = ftplib.FTP(serverAddr)
            session.login(USER_NAME_TO_SERVERS, PASSWORD_TO_SERVERS)
            session.cwd(NODES_DIRECTORY_PATH_SERVER)

            # start of ugly code - be sure that the i_TransmissionType was deleted
            SSHCmd = "sshpass -p " + PASSWORD_TO_SERVERS + " ssh " + USER_NAME_TO_SERVERS + "@" + serverAddr + " "
            os.system(SSHCmd)
            # end of ugly code
            session.cwd(NODES_DIRECTORY_PATH_SERVER + i_TransmissionType)  # enter to this folder


            file = open(EMANE_HOST_PATH + ServerIPNodeID)
            session.storbinary('STOR ' + ServerIPNodeID, file)  # copy file from host to server
            session.sendcmd('SITE CHMOD 777 ' + ServerIPNodeID)  # chmod the file
            file.close()
            os.remove(EMANE_HOST_PATH + ServerIPNodeID)
            session.quit()
            id+=1

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

    def spreadPlatforms(self, i_IpPath, i_TransmissionType):
        '''
        The method can't spread folders.
        The method spread all the folder from the host to all the servers.
        Then on every server deletes the files that should not be there (only by the node ID).
        :param i_IpPath:
        :return:
        '''
        if i_TransmissionType == '3':
            nodesDirPathHost = NODES_DIRECTORY_PATH_HOST_TDMA
        elif i_TransmissionType == 'tdmact':
            nodesDirPathHost = NODES_DIRECTORY_PATH_HOST_TDMACT
        for server in i_IpPath:
            nodeID = []
            serverAddr = server[0]
            session = ftplib.FTP(serverAddr)
            session.login(USER_NAME_TO_SERVERS, PASSWORD_TO_SERVERS)
            session.cwd(NODES_DIRECTORY_PATH_SERVER)
            # start of ugly code - be sure that the i_TransmissionType was deleted
            SSHCmd = "sshpass -p " + PASSWORD_TO_SERVERS + " ssh -p 22 " + USER_NAME_TO_SERVERS + "@" + serverAddr + " "
            bridgeCmd = SSHCmd +  "echo " + PASSWORD_TO_SERVERS + " | sudo -S " + 'rm -Rf ' + NODES_DIRECTORY_PATH_SERVER + i_TransmissionType + " &"
            os.system(bridgeCmd)
            # end of ugly code
            try:
                self.ftpRemoveTree(session, i_TransmissionType)  # delete the exist folder, if it is.
            finally:
                None
            session.mkd(i_TransmissionType)  # make folder
            session.sendcmd('SITE CHMOD 777 ' + i_TransmissionType)  # chahnge mode of this folder to 777
            session.cwd(NODES_DIRECTORY_PATH_SERVER + i_TransmissionType)  # enter to this folder
            hostFiles = [f for f in listdir(nodesDirPathHost) if isfile(join(nodesDirPathHost, f))]
            # list all files that in host's folder
            for fileName in hostFiles:
                file = open(nodesDirPathHost + fileName)
                session.storbinary('STOR ' + fileName, file)  # copy all files from host to server
                session.sendcmd('SITE CHMOD 777 ' + fileName)  # chmod the files
                file.close()

            listfile = ['demo-start', 'demo-stop', 'protocol-config.xsd', 'emanelayerdlep.xml', 'emanelayersnmp.xml', 'emanelayerfilter.xml', 'eventservice.xml', 'eventdaemon.xml', 'transraw.xml', 'tdmamac.xml', 'credit-windowing-03.xml', 'dlep-draft-24.xml', 'dlep-rfc-8175.xml', 'schedule.xml']
            flag = 0
            for i in server[1]:  # make list with all the nodes ID's
                n = re.findall('\d+', str(i))[-1]
                nodeID.append(int(n))
            namesAllFiles = session.nlst()
            for testFile in namesAllFiles:  # in the server, nodes that not in the list of the nodes ID, will be deleted.
                fn = re.findall('\d+', str(testFile))
                if fn:
                    fn = int(fn[-1])
                    if fn not in nodeID:
                        for file in listfile:
                            if testFile == file:
                                flag = 1
                        if flag == 0:
                            session.delete(testFile)
                flag = 0
                            
            session.quit()

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
            #FTP Server error
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
    def __init__(self, i_TransmissionType, i_NumberOfNodes):
        self.transmissionType = i_TransmissionType
        self.numberOfNodes = i_NumberOfNodes
        #self.outputPath = i_OutputPath
        self.StartHatching(self.transmissionType, self.numberOfNodes)

    def StartHatching(self, i_TransmiionType, i_NumberOfNodes):
        self.hatchRouting(i_TransmiionType, i_NumberOfNodes)
        self.hatchTDMAPlatform(i_TransmiionType, i_NumberOfNodes)
        self.hatchTDMANEM(i_TransmiionType, i_NumberOfNodes)
        self.hatchFileAlways(i_TransmiionType)
        if i_TransmiionType == 'tdmact':
            self.hatchTDMACTNem(i_NumberOfNodes)

    def hatchFileAlways(self, i_TransmiionType):
        pathWhereFind = None
        listfile = []
        listfile = ['emanelayerdlep', 'emanelayersnmp', 'emanelayerfilter', 'eventservice', 'eventdaemon', 'transraw', 'tdmamac', 'credit-windowing-03', 'dlep-draft-24', 'dlep-rfc-8175', 'schedule']
        if i_TransmiionType == '3':
            pathWhereFind = EMANE_HOST_PATH + '3/'
            #self.removeFile(pathWhereFind, 'emanelayerdlep')
        #pathWhereFind = EMANE_HOST_PATH + '3/'
        replacements = []
        for file in listfile:
            template = EMANE_TEMPLATES_PATH_HOST + file + '.xml.template'
            pathToNewFiles = pathWhereFind + file + '.xml'
            self.preprocess(replacements, template, pathToNewFiles)
        template = EMANE_TEMPLATES_PATH_HOST + 'protocol-config.xsd.template'
        pathToNewFiles = pathWhereFind + 'protocol-config.xsd'
        self.preprocess(replacements, template, pathToNewFiles)
        replacements = [["DEMOID", i_TransmiionType]]
        template = EMANE_TEMPLATES_PATH_HOST + 'demo-start.template'
        pathToNewFiles = pathWhereFind + 'demo-start'
        self.preprocess(replacements, template, pathToNewFiles)
        replacements = [["DEMOID", i_TransmiionType]]
        template = EMANE_TEMPLATES_PATH_HOST + 'demo-stop.template'
        pathToNewFiles = pathWhereFind + 'demo-stop'
        self.preprocess(replacements, template, pathToNewFiles)

    def hatchTDMAPlatform(self, i_TransmissionType, i_NumberOfNodes):
        pathWhereFind = None
        nemxmlChangeTo = None
        if i_TransmissionType == '3':
            pathWhereFind = EMANE_HOST_PATH + '3/'
            self.removeFile(pathWhereFind, 'platform')
        elif i_TransmissionType == 'tdmact':
            pathWhereFind = EMANE_HOST_PATH + 'tdmact_scenario/'
            self.removeFile(pathWhereFind, 'platform')
        for index in range(1, i_NumberOfNodes):
           if i_TransmissionType == '3':
               nemxmlChangeTo = 'tdmanem' + str(index) + '.xml'
               pathWhereFind = EMANE_HOST_PATH + '3/'
           elif i_TransmissionType == 'tdmact':
               pathWhereFind = EMANE_HOST_PATH + 'tdmact_scenario/'
               nemxmlChangeTo = 'tdmactnem' + str(index) + '.xml'
           replacements = [["NEMID", str(index)], ["NEMXML", nemxmlChangeTo]]
           template = EMANE_TEMPLATES_PATH_HOST + 'platform.xml.template'
           pathToNewFiles = pathWhereFind + 'platform' + str(index) + '.xml'
           self.preprocess(replacements, template, pathToNewFiles)

    def hatchTDMANEM(self, i_TransmissionType, i_NumberOfNodes):
        pathWhereFind = None
        nemxmlChangeTo = None
        if i_TransmissionType == '3':
            pathWhereFind = EMANE_HOST_PATH + '3/'
            self.removeFile(pathWhereFind, 'tdmanem')
        for index in range(1, i_NumberOfNodes):
           if i_TransmissionType == '3':
               pathWhereFind = EMANE_HOST_PATH + '3/'
               hexa = '{:X}'.format(index)
           if index <= 15:
                replacements = [["NEMID", str(index)], ["HEX", '0' + str(hexa)], ["ipAdressRedis", "10.99.1.1"]]
           else:
                replacements = [["NEMID", str(index)], ["HEX", str(hexa)], ["ipAdressRedis", "10.99.1.1"]]
           template = EMANE_TEMPLATES_PATH_HOST + 'tdmanem.xml.template'
           pathToNewFiles = pathWhereFind + 'tdmanem' + str(index) + '.xml'
           self.preprocess(replacements, template, pathToNewFiles)

    def hatchTDMACTNem(self, i_NumberOfNodes):
        pathWhereFind = EMANE_HOST_PATH + 'tdmact_scenario/'
        self.removeFile(pathWhereFind, 'tdmactnem')
        for index in range(1, i_NumberOfNodes):
            replacements = [["NODEID", index]]
            template = EMANE_TEMPLATES_PATH_HOST + 'tdmactnem.xml.template'
            i_PathToNewFiles = pathWhereFind + 'tdmactnem' + str(index) + '.xml'
            self.preprocess(replacements, template, i_PathToNewFiles)

    def hatchRouting(self, i_TransmissionType, i_NumberOfNodes):
        pathWhereFind = None
        fileName = 'routing'
        if i_TransmissionType == '3':
            pathWhereFind = EMANE_HOST_PATH + '3/'
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
        for file in os.listdir(i_Path):
            if file.startswith(i_NameOfFile):
                file = i_Path + '/' + file
                os.remove(file)

def usage():
    print'To create nodes run: '
    print'   python configureServer.py create [3/tdmact] number_of_nodes'
    print'To spread the nodes to the servers:'
    print'   First, write correct ''nodes2s.txt'' file, then run:'
    print'   python configureServer.py spread [3/tdmact (which nodes to spread)]'

if __name__ == '__main__':
    if len(sys.argv) == 3 or len(sys.argv) == 4:
        if sys.argv[1] == 'create':
            transmissionType = sys.argv[2]
            numberOfNodes = sys.argv[3]
            hatchNodes(transmissionType, int(numberOfNodes)+1)
        elif sys.argv[1] == 'spread':
            transmissionType = sys.argv[2]
            work = spreadToServers()
            listSrvNodes = work.readFile()
            #listPath = work.getPlatformPath(listSrvPlat)
            addrPath = work.matchAddresses(listSrvNodes)
            SSHCmd = "sshpass -p " + PASSWORD_TO_SERVERS + " ssh -p 22 " + USER_NAME_TO_SERVERS + "@" + "192.168.43.172" + " "
            bridgeCmd = SSHCmd +  "echo " + PASSWORD_TO_SERVERS + " | sudo -S " + 'rm -Rf ' + NODES_DIRECTORY_PATH_SERVER + '3' + " &"
            print bridgeCmd
            work.spreadPlatforms(addrPath, transmissionType)
            work.spreadServersIP(addrPath, transmissionType)

            work.txtFile.close()
    else:
        print "Illegal number of parameters."
        usage()
