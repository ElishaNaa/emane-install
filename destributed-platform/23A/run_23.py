#
# ........ FAST & DIRTY ........
#
import numpy as np
import os
import subprocess
import time
from contextlib import contextmanager
import map_tools
import socket

import re

from emanesh.events import EventService
from emanesh.events import PathlossEvent
from emanesh.events import LocationEvent
from emanesh import ControlPortClient
from emanesh import EMANEShell


import radio_calculations

from namedlist import namedlist

LOCATION_SOLDIER_1_LAT = 31.1643
LOCATION_SOLDIER_1_LON = 34.5285
LOCATION_SOLDIER_2_LAT = 31.1643
LOCATION_SOLDIER_2_LON = 34.5324
LOCATION_SOLDIER_3_LAT = 31.1639
LOCATION_SOLDIER_3_LON = 34.5320
LOCATION_SOLDIER_4_LAT = 31.1630
LOCATION_SOLDIER_4_LON = 34.5313
LOCATION_SOLDIER_5_LAT = 31.1633
LOCATION_SOLDIER_5_LON = 34.5334
LOCATION_SOLDIER_6_LAT = 31.1630
LOCATION_SOLDIER_6_LON = 34.5338
LOCATION_PLANE_GROUND_LAT = 31.2256
LOCATION_PLANE_GROUND_LON = 34.6784
LOCATION_PLANE_AIR_LAT = 31.2363
LOCATION_PLANE_AIR_LON = 34.6285

project_dir = "/home/ubuntu/Desktop/emane-tutorial/23aliakim/"

@contextmanager
def open_23():
    # subprocess.call(["sudo", "-S", "/home/ubuntu/Desktop/two_points/start"], shell=False) best but annoying to type the password..
    subprocess.call(["cd /home/ubuntu/Desktop/emane-tutorial/23aliakim/; echo cisco | sudo -S ./demo-start"], shell=True)
    #time.sleep(2)
    #subprocess.call(["cd /home/ubuntu/Desktop/emane-tutorial/23aliakim/; echo cisco | sudo -S ./mgenstat"], shell=True)
    try:
        yield
    finally:
        # stop emane
        subprocess.call(["cd /home/ubuntu/Desktop/emane-tutorial/23aliakim/; echo cisco | sudo -S ./demo-stop"], shell=True)
        # subprocess.call(["sudo", "-S", "/home/ubuntu/Desktop/two_points/end"], shell=False)


@contextmanager
def open_sdt():
    # subprocess.call(["sudo", "-S", "/home/ubuntu/Desktop/two_points/start"], shell=False) best but annoying to type the password..
    subprocess.call(["cd /home/ubuntu/Desktop/emane-tutorial/23aliakim/; echo cisco | sudo -S ./demo-start"], shell=True)
    try:
        yield
    finally:
        # stop emane
        subprocess.call(["cd /home/ubuntu/Desktop/emane-tutorial/23aliakim/; echo cisco | sudo -S ./demo-stop"], shell=True)


NEM = namedlist('NEM', 'id latitude longitude height')
radioNEMs = []
a2gNEM7 = NEM(7, LOCATION_PLANE_AIR_LAT, LOCATION_PLANE_AIR_LON, 0) #CHANGED GROUND-> AIR
a2gNEM8 = NEM(8, LOCATION_SOLDIER_3_LAT, LOCATION_SOLDIER_3_LON, 1.5)
a2gNEM9 = NEM(9, LOCATION_SOLDIER_6_LAT, LOCATION_SOLDIER_6_LON, 1.5)
a2gNEM13 = NEM(13, LOCATION_SOLDIER_1_LAT, LOCATION_SOLDIER_1_LON, 1.5)
a2gNEM14 = NEM(14, LOCATION_SOLDIER_2_LAT, LOCATION_SOLDIER_2_LON, 1.5)
a2gNEM15 = NEM(15, LOCATION_SOLDIER_4_LAT, LOCATION_SOLDIER_4_LON, 1.5)
a2gNEM16 = NEM(16, LOCATION_SOLDIER_5_LAT, LOCATION_SOLDIER_5_LON, 1.5)

default_p_radio = dict(f_Hz=5e9, ERP_W=4, protocol=1)
default_p_a2g = dict(f_Hz=2.5e9, ERP_W=45, protocol=2)

sdtcmd_path = "/home/ubuntu/Software/sdt-linux-amd64-2.1/sdtcmd"


def freespace_pathloss(tx, rx):
    f = 600e6 # Hertz
    c = 299792458.0
    dist = map_tools.distance_elevated(tx.latitude, tx.longitude, rx.latitude, rx.longitude)
    return 20*np.log10(np.pi*4*dist*f/c)

def freespace_pathloss_a2g(tx,rx,p=default_p_a2g):
    height_m = 500 # plane height
    f = p['f_Hz'] # Hertz
    c = 299792458.0
    txh_m = p['txh_m']
    rxh_m = p['rxh_m']
    dist = map_tools.distance_elevated_h(tx.latitude, tx.longitude, txh_m, rx.latitude, rx.longitude, rxh_m)
    return 20*np.log10(np.pi*4*dist*f/c)

def egli_pathloss(tx,rx,p=default_p_radio):
    f_MHz = p['f_Hz']/1e6 # MHz
    dist_m = map_tools.distance_elevated(tx.latitude, tx.longitude, rx.latitude, rx.longitude)
    dist_km = dist_m/1000 # km
    ant_height_m = 2 # m
    return 88 + 20*np.log10(f_MHz)+40*np.log10(dist_km)-20*np.log10(ant_height_m*ant_height_m)

def egli_los_pathloss(tx,rx,p=default_p_radio):
    f_MHz = p['f_Hz']/1e6 # MHz
    c = 299792458.0
    dist_m = map_tools.distance_elevated(tx.latitude, tx.longitude, rx.latitude, rx.longitude)
    dist_km = dist_m/1000 # km
    ant_height_m = 2 # m
    if dist_km <= 0.5:
        wvlength = c/(f_MHz*1e6)
        acs_dB = 12.66-3.5*np.log10(ant_height_m/wvlength)+0.07*dist_km
        fsl_pl_dB = 32.45 + 20*np.log10(f_MHz)+20*np.log10(dist_km)
        return acs_dB + fsl_pl_dB
    else:
        return 88 + 20*np.log10(f_MHz)+40*np.log10(dist_km)-20*np.log10(ant_height_m*ant_height_m)

def tworay_pathloss(tx,rx,p=default_p_a2g):
    f_MHz = p['f_Hz']/1e6 # MHz
    dist_m = map_tools.distance_elevated(tx.latitude, tx.longitude, rx.latitude, rx.longitude)
    rx_height_m = p['rxh_m'] # m
    tx_height_m = p['txh_m']
    return 40*np.log10(dist_m) - 20*np.log10(rx_height_m*tx_height_m)

def tworay_real_pathloss(tx,rx,p=default_p_a2g):
    # http://web.cs.ucdavis.edu/~liu/289I/Material/book-goldsmith.pdf, p. 32
    # basic geometry in https://www.mathworks.com/help/phased/ref/phased.tworaychannel-class.html
    # NO ELEVATION in this model
    # vertical polarization
    f_MHz = p['f_Hz']/1e6 # MHz
    c = 299792458.0
    lambda_m = c/p['f_Hz']
    rx_height_m = float(p['rxh_m']) # m
    tx_height_m = float(p['txh_m'])
    l_m = map_tools.distance_h(tx.latitude, tx.longitude, tx_height_m, rx.latitude, rx.longitude, rx_height_m)
    d_m = map_tools.distance(tx.latitude, tx.longitude, rx.latitude, rx.longitude)
    r_m = (tx_height_m/(rx_height_m+tx_height_m))*np.sqrt((rx_height_m+tx_height_m)**2 +d_m**2)
    rprime_m = (rx_height_m/(rx_height_m+tx_height_m))*np.sqrt((rx_height_m+tx_height_m)**2 +d_m**2)
    dphi = 2*np.pi*(r_m+rprime_m-l_m)/lambda_m
    eps_r = 15 # ground
    theta = np.arcsin(rx_height_m/rprime_m)
    Z = np.sqrt(eps_r-np.cos(theta)**2)/eps_r # vertical polarization
    R = (np.sin(theta)-Z)/(np.sin(theta)+Z)
    #return dphi-
    return -10*np.log10(( (lambda_m/(4*np.pi))**2 *np.absolute(1/l_m + R*np.exp(1j*dphi)/(r_m+rprime_m))**2))
    #return -10 * np.log10(
     #   (lambda_m / (4 * np.pi) ** 2 * np.absolute(1 / d_m + -1* np.exp(1j * 4*np.pi*rx_height_m*tx_height_m/(lambda_m*d_m)) / (d_m)) ** 2))

def signalserver_pathloss(tx,rx,p):
    # power of transmitter: 35 W
    # frequency 600 MHz
    # model (-pm) irregular terrain simple
    # antenna heights (-txh -rxh): 2 m
    f_MHz = p['f_Hz']/1e6
    ERP_W = p['ERP_W']
    protocol = p['protocol']
    ssdir = os.path.join(project_dir, "persist/signalserver")
    if not os.path.exists(ssdir):
        os.makedirs(ssdir)
    out_file = os.path.join(ssdir, ("o%d_%d.hd" % (tx.id, rx.id)))
    out = subprocess.check_output(["/home/ubuntu/Software/Signal-Server/signalserverHD", "-m","-dbm",
                             "-sdf", "/home/ubuntu/Python/maps/SRTM3GL1/",
                             "-udt", "/home/ubuntu/Python/maps/udt/aliakim.udt",
                             "-erp", str(ERP_W),
                             "-f", str(f_MHz),
                             "-pm", str(protocol), # 1: ITM, 2: LOS, 3: Hata, 4: ECC33, 5: SUI, 6: COST-Hata, 7: FSPL, 8: ITWOM, 9: Ericsson, 10: Plane earth, 11: Egli VHF/UHF
                             "-txh", str(tx.height),
                             "-rxh", str(rx.height),
                             "-lat", str(tx.latitude),
                             "-lon", str(tx.longitude),
                             "-rla", str(rx.latitude),
                             "-rlo", str(rx.longitude),
                             "-o", out_file], cwd=ssdir)

    #os.chmod(out_file + ".tx", 777)
    f = open(out_file + ".txt", "r")
    s = f.read()
    #os.remove(out_file + ".tx")
    m = re.search(r'Computed path loss: (.+) dB', s)
    pl = float(m.group(1))
    return pl

def splat_pathloss(tx,rx,p=default_p_radio):
    # power of transmitter: 25 W
    # frequency 600 MHz
    # model (-pm) irregular terrain simple
    # antenna heights (-txh -rxh): 2 m
    f_MHz = p['f_Hz']/1e6
    ERP_W = p['ERP_W']
    ssdir = os.path.join(project_dir, "persist/splat")
    if not os.path.exists(ssdir):
        os.makedirs(ssdir)
    tx_path = os.path.join(ssdir, ("tx%d_%d.qth" % (tx.id, rx.id)))
    rx_path = os.path.join(ssdir, ("rx%d_%d.qth" % (tx.id, rx.id)))
    lrp_path = os.path.join(ssdir, ("tx%d_%d.lrp" % (tx.id, rx.id)))

    path_splathd = '/usr/local/bin/splat-hd'
    path_mapdir = '/home/ubuntu/Python/maps/SRTM3GL1'

    tx_file = open(tx_path, "w")
    rx_file = open(rx_path, "w")
    params_file = open(lrp_path, "w")

    tx_file.write('transmitter\n%g\n%g\n%g meters\n' % (tx.latitude, -tx.longitude, 2))  # SPLAT! convention for longitude
    rx_file.write('receiver\n%g\n%g\n%g meters\n' % (rx.latitude, -rx.longitude, 2))  # SPLAT! convention for longitude

    params_file.write('''15.000 ; Earth Dielectric Constant (Relative permittivity)
    % 0.005 ; Earth Conductivity (Siemens per meter)
    % 301.000 ; Atmospheric Bending Constant (N-units)
''')
    params_file.write(("%% %f ; Frequency in MHz (20 MHz to 20 GHz)" % (f_MHz)))
    params_file.write('''% 5 ;Radio Climate (5 = Continental Temperate)
    % 0 ;Polarization (0 = Horizontal, 1 = Vertical)
    % 0.50 ; Fraction of situations (50% of locations)
    % 0.90 ; Fraction of time (90% of the time)
    % 35.0 ; ERP in Watts (optional)
''')
    params_file.close()
    tx_file.close()
    rx_file.close()
    out = subprocess.check_output([path_splathd,
                                   "-f", str(f_MHz),
                                    "-t", tx_path,
                                    '-r', rx_path,
                                    '-o', ('test_%d_%d.ppm' % (tx.id, rx.id)),
                                    '-d', path_mapdir,
                                    '-p', ('terrain_profile_%d_%d.png' % (tx.id, rx.id)),
                                    '-l', ('loss_%d_%d.png' % (tx.id, rx.id)),
                                   '-dbm',
                                   '-metric',
                                   '-L 2'],
                                    shell=False, cwd=ssdir)
    os.rename(os.path.join(ssdir,"transmitter-site_report.txt"), os.path.join(ssdir,("transmitter-site_report_%d_%d.txt" % (tx.id, rx.id))))
    os.rename(os.path.join(ssdir,"transmitter-to-receiver.txt"), os.path.join(ssdir,("transmitter--to-receiver_%d_%d.txt" % (tx.id, rx.id))))

    f = open(os.path.join(ssdir,("transmitter--to-receiver_%d_%d.txt" % (tx.id, rx.id))) , "r")
    s = f.read()
    m = re.search(r'ITWOM Version 3.0 path loss: (.+) dB', s)
    pl = float(m.group(1))
    return pl

def message_sdt(s):
    os.system(sdtcmd_path + " " + s)

def init_nodes(service):
    #send schedule
    os.system("emaneevent-tdmaschedule /home/ubuntu/Desktop/emane-tutorial/23aliakim/schedule.xml -i emanenode0")
    #message_sdt(b"reset")
    message_sdt(b"")
    message_sdt(b"bgbounds 34.54,31.15,34.8,31.26")

    message_sdt(("sprite base_station image %s size 52,25" % (os.path.join(project_dir,'base_station.png'))))
    message_sdt(b"node node-01 type base_station label blue")
    message_sdt(b"node node-01 position 34.7572,31.2414")

    message_sdt(("sprite plane image %s size 52,25" % (os.path.join(project_dir,'plane.png'))))
    message_sdt(b"node node-02 type plane label blue")
    #cmdString = "b\"node node-02 position " + str(LOCATION_AIRPORT_LON) + "," + str(LOCATION_AIRPORT_LAT) + "\""
    message_sdt(b"node node-02 position 34.6784,31.2256")
    message_sdt(b"link node-01,node-02,wifi")

    message_sdt(("sprite soldier image %s size 52,25" % (os.path.join(project_dir,'soldier.gif'))))
    message_sdt(b"node node-07 type soldier label blue")
    #cmdString = "b\"node node-07 position " + str(LOCATION_SOLDIER_1_LON) + "," + str(LOCATION_SOLDIER_1_LAT) + "\""
    message_sdt(b"node node-07 position 34.5285,31.1643")
    message_sdt(b"node node-08 type soldier label blue")
    #cmdString = "b\"node node-08 position " + str(LOCATION_SOLDIER_2_LON) + "," + str(LOCATION_SOLDIER_2_LAT) + "\""
    message_sdt(b"node node-08 position 34.5324,31.1643")
    message_sdt(b"node node-03 type soldier label blue")
    #cmdString = "b\"node node-03 position " + str(LOCATION_SOLDIER_3_LON) + "," + str(LOCATION_SOLDIER_3_LAT) + "\""
    message_sdt(b"node node-03 position 34.5320,31.1639")
    message_sdt(b"node node-04 type soldier label blue")
    #cmdString = "b\"node node-04 position " + str(LOCATION_SOLDIER_4_LON) + "," + str(LOCATION_SOLDIER_4_LAT) + "\""
    message_sdt(b"node node-04 position 34.5313,31.1630")
    message_sdt(b"node node-05 type soldier label blue")
    #cmdString = "b\"node node-05 position " + str(LOCATION_SOLDIER_5_LON) + "," + str(LOCATION_SOLDIER_5_LAT) + "\""
    message_sdt(b"node node-05 position 34.5334,31.1633")
    message_sdt(b"node node-06 type soldier label blue")
    #cmdString = "b\"node node-06 position " + str(LOCATION_SOLDIER_6_LON) + "," + str(LOCATION_SOLDIER_6_LAT) + "\""
    message_sdt(b"node node-06 position 34.5338,31.1630")

    # create an event setting base station position
    event = LocationEvent()
    event.append(11, latitude=31.2414, longitude=34.7572, altitude=15.000000)
    service.publish(0, event)
    # create an event setting plane position
    event = LocationEvent()
    event.append(12, latitude=LOCATION_PLANE_GROUND_LAT, longitude=LOCATION_PLANE_GROUND_LON, altitude=0.000000)
    service.publish(0, event)

    #
    # # set 1 men position
    # event = LocationEvent()
    radioNEMs.append(NEM(1, LOCATION_SOLDIER_1_LAT, LOCATION_SOLDIER_1_LON, 1.5))
    radioNEMs.append(NEM(2, LOCATION_SOLDIER_2_LAT, LOCATION_SOLDIER_2_LON, 1.5))
    radioNEMs.append(NEM(3, LOCATION_SOLDIER_3_LAT, LOCATION_SOLDIER_3_LON, 1.5))

    # # set 2 men position
    # event = LocationEvent()
    radioNEMs.append(NEM(4, LOCATION_SOLDIER_4_LAT, LOCATION_SOLDIER_4_LON, 1.5))
    radioNEMs.append(NEM(5, LOCATION_SOLDIER_5_LAT, LOCATION_SOLDIER_5_LON, 1.5))
    radioNEMs.append(NEM(6, LOCATION_SOLDIER_6_LAT, LOCATION_SOLDIER_6_LON, 1.5))

    update_radio_pathlosses(service)
    update_8_9_13_14_15_16_pathloss(service)
    update_a2g_pathlosses(service)
    #time.sleep(20)


def update_radio_pathlosses(service):
    p = default_p_radio
    max_id = max([i.id for i in radioNEMs])
    matrix = np.zeros((max_id+1, max_id+1))
    for nemtx in radioNEMs:
        event = PathlossEvent()
        for nemrx in radioNEMs:
            if (nemrx.id != nemtx.id):
                pl = signalserver_pathloss(nemtx,nemrx,p)
                matrix[nemtx.id][nemrx.id] = pl
                event.append(nemrx.id, forward=pl)
        service.publish(nemtx.id, event)

    print "actual:"
    print np.array_str(matrix,max_line_width=200, precision=5)

    for nem in radioNEMs:
        print nem.id, nem.latitude, nem.longitude
    
    map_tools.draw_elevation([32.7, 33.1], [34.9, 35.2], ([x.longitude for x in radioNEMs],[x.latitude for x in radioNEMs], [str(x.id) for x in radioNEMs]))

def update_8_9_13_14_15_16_pathloss(service):
    event = PathlossEvent()
    radios = (8, 9, 13, 14, 15, 16)
    pl = 9999  # just high number
    for radio in radios:
        event.append(radio, forward=pl)
        nextradio = radios.index(radio)+1
        for nextradio in radios:
            if radio != nextradio:
                service.publish(nextradio, event)
                print ("Pathloss " + str(radio) + " -> " + str(nextradio) + " : " + str(pl))


def update_a2g_pathlosses(service):
    p = default_p_a2g
    radiosNEM = (a2gNEM8, a2gNEM9, a2gNEM13, a2gNEM14, a2gNEM15, a2gNEM16)
    radios = (8, 9, 13, 14, 15, 16)
    for radio in radiosNEM:
        pl = signalserver_pathloss(a2gNEM7, radio, p)
        event = PathlossEvent()
        event.append(radios[radiosNEM.index(radio)], forward=pl)
        service.publish(7, event)
        event = PathlossEvent()
        event.append(7, forward=pl)
        service.publish(radios[radiosNEM.index(radio)], event)


def move_node(service, coordinates):
    for i in range(0, len(coordinates)):
        if i == 0:
            continue
        # if i==1 or i==2 or i== 20 or i==21:
        #     n = 4
        #     if i==1 or 1==21:
        #         height = 0.0
        else:
            n = 10
            height = 1800.0
            a2gNEM7.height = 1800
        for counter in range(0, n):
            counter = float(counter)/n
            lon = coordinates[i-1][0] + counter * (coordinates[i][0] - coordinates[i-1][0])
            lat = coordinates[i-1][1] + counter * (coordinates[i][1] - coordinates[i-1][1])
            message_sdt("node node-02 position" + " " + str(lon)[0:7] + "," + str(lat)[0:7] + "")
            message_sdt(b"wait 50.0")
            a2gNEM7.latitude = lat
            a2gNEM7.longitude = lon
            # create an event setting plane position only for NEM 12 (comm is precomputed)
            event = LocationEvent()
            event.append(12, latitude=lat, longitude=lon, altitude=height)
            service.publish(0, event)

            update_a2g_pathlosses(service)
            time.sleep(0.01)
        time.sleep(5)

def check_models():
    N = 150
    x = np.logspace(-5,1,N)
    d = np.zeros(N)
    fs = np.zeros(N)
    fa = np.zeros(N)
    eg = np.zeros(N)
    el = np.zeros(N)
    tr = np.zeros(N)
    trr = np.zeros(N)
    tx = NEM(0,0,1)
    rx = NEM(0,0,1)

    for i in range(N):
        d[i] = map_tools.distance(0,0,x[i],0)
        rx.latitude = x[i]
        fs[i] = freespace_pathloss(tx,rx)
        fa[i] = freespace_pathloss_a2g(tx, rx)
        eg[i] = egli_pathloss(tx, rx)
        el[i] = egli_los_pathloss(tx,rx)
        tr[i] = tworay_pathloss(tx,rx)
        trr[i] = tworay_real_pathloss(tx,rx)
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots()
    line1 = ax.semilogx(d, fs, label='FS radio')
    line2 = ax.semilogx(d, eg, label='EGLI radio')
    line3 = ax.semilogx(d, el, label='EGLI-los radio')
    line4 = ax.semilogx(d, tr, label='Two-ray a2g')
    line5 = ax.semilogx(d, fa, label='FS a2g')
    line6 = ax.semilogx(d, trr, label='Two-ray real a2g')
    ax.legend(loc='lower right')
    plt.show()

    NEM8 = NEM(33.010963, 35.088425, 8) # height 8
    NEM9 = NEM(33.015844, 35.147439, 9)
    NEM7 = NEM(32.9185293225, 35.083672245, 7) # h: 500 m

if __name__ == "__main__":

    #check_models()
    #raw_input("Enter to continue:")

    with open_23():
        time.sleep(5)
        esh = EMANEShell('localhost',47000)
        # Debug level
        #esh.do_loglevel('4')

        coordinates = [#(34.6784, 31.2256), (34.6285, 31.2363), #airport air
                       (34.5375, 31.1614), (34.5376, 31.1644), (34.5350, 31.1646), (34.5298, 31.1651), (34.5277, 31.1637), (34.5316, 31.1622), #6 node flight roude
                       (34.5375, 31.1614), (34.5376, 31.1644), (34.5350, 31.1646), (34.5298, 31.1651), (34.5277, 31.1637), (34.5316, 31.1622), #6 node flight roude
                       (34.5375, 31.1614), (34.5376, 31.1644), (34.5350, 31.1646), (34.5298, 31.1651), (34.5277, 31.1637), (34.5316, 31.1622), #6 node flight roude
                       (34.6285, 31.2363), (34.6755, 31.2260)] #airport ground
        service = EventService(('224.1.2.8', 45703, 'emanenode0'))

        if True:
             sdtproc = subprocess.Popen(["/home/ubuntu/Software/sdt-linux-amd64-2.1/sdt3d/sdt3d.sh"])
             time.sleep(15)

        init_nodes(service)
        raw_input("Press Enter to start flying...")
        move_node(service, coordinates)
        print "Done"
        #pyplot.plot(distances, rates)
        #pyplot.xscale('log')
        #pyplot.show()