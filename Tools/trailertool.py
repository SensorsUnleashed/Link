#!/usr/bin/env python

import sys, binascii,datetime,argparse
from _datetime import time
import time as t

def get_version(versionstr):
    versionsplit = versionstr.split(".");
    version = int(versionsplit[0]).to_bytes(1, byteorder='little', signed=False)
    version += int(versionsplit[1]).to_bytes(1, byteorder='little', signed=False)
    version += int(versionsplit[2]).to_bytes(1, byteorder='little', signed=False)
    return version;

def get_crc(data):
    crc32 = binascii.crc32(data) & 0xffffffff
    return crc32

def clean_hex(data):
    if data.startswith('0x'):
        data = data[2:]
    if data.endswith('ULL'):
        data = data[:-3]
    return data

parser = argparse.ArgumentParser(description='Parse and create trailers.')
parser.add_argument("-vV", action="store_true", help="print stored fileinfo")
parser.add_argument("-vS", action="store_true", help="print image start")
parser.add_argument("-vC", action="store_true", help="print CRC")
parser.add_argument("-vP", action="store_true", help="print product type")
parser.add_argument("-v",  action="store_true", help="verbose output")
parser.add_argument("-E", help="image end filesize");
parser.add_argument("-V", help="image version eg. 1.0.0");
parser.add_argument("-A", help="image start address");
parser.add_argument("-P", help="product type eg. 1 for debugdevice");
parser.add_argument("-T", help="radio hw target eg. 2 for radioone");
parser.add_argument("-i", help="input file");
parser.add_argument("-o", help="output file");

args = parser.parse_args()

if (args.i):
    file = open(args.i, 'rb') 
else:
    exit

data = file.read()
file.close();

#Print information about file
if args.vV:
        #product type before the trailer
    pos = -28
    print("Header version: ", data[pos]);
    pos = pos + 1;        
    
    #reserved1
    pos = pos + 1;
    
    print("Product type: ", int.from_bytes(data[pos : pos + 2], byteorder='little', signed=False));
    pos = pos + 2;
                
    print("Created: ", t.strftime("%a, %d %b %Y %H:%M:%S %Z", t.localtime(int.from_bytes(data[pos : pos + 8], byteorder='little', signed=False))))
    pos = pos + 8;                     
    
    print("Radio Target hw id: ", data[pos]);
    pos = pos + 1;  

    # version
    print("Version: ", data[pos], ".", data[pos + 1], ".", data[pos + 2]);
    pos = pos + 3

    # start addr of image
    print("Start addr: ", hex(int.from_bytes(data[pos : pos + 4], byteorder='little', signed=False)))
    pos = pos + 4
    
    #reserved2
    pos = pos + 4;
    
    print("CRC: ", hex(int.from_bytes(data[pos : ], byteorder='little', signed=False)));
    raise SystemExit
    
else:
    if len(data) > 102400:
        print("Filesize too high", len(data));
        raise SystemExit

    
    reqOk = True;
    if args.E:
        endfilesize = int(args.E);
    else:
        print("Missing endfile size '-E'");
        reqOk = False;
    if args.P:
        product = int(args.P).to_bytes(2, byteorder='little', signed=False)
    else:
        print("Missing product type '-P'");
        reqOk = False;
    if args.V:
        version = get_version(args.V)
    else:
        print("Missing version '-V'");
        reqOk = False;
    if args.A:
        image_start = int(args.A, 16).to_bytes(4, byteorder='little', signed=False);
    else:
        print("Missing start address '-A'");
        reqOk = False;
    if args.T:
        radiotarget = int(args.T).to_bytes(1, byteorder='little', signed=False);
    else:
        print("Missing target hw info '-T'");
        reqOk = False;       
    
    if not reqOk: 
        raise SystemExit     
          
    headerversion = int(1).to_bytes(1, byteorder='little', signed=False);
    epoch_time = int(t.time()).to_bytes(8, byteorder='little', signed=False)
    reserved1 = int(0xFF).to_bytes(1, byteorder='little', signed=False);   
    reserved2 = int(0xFFFFFFFF).to_bytes(4, byteorder='little', signed=False);   
    #The trailer is arranged so that there is no padding, and it later can be cast to a struct
    trailer = headerversion + reserved1 + product + epoch_time + radiotarget + version + image_start + reserved2
        
    if args.v:
        print("Trailer:", binascii.hexlify(trailer), len(trailer))
        
    #Pad the image before adding the trailer
    padsize = endfilesize - (len(data) + len(trailer) + 4)  #4=crc
    pad = bytes(padsize);
        
    data = data + pad + trailer
    crc = get_crc(data);
    data = data + crc.to_bytes(4, byteorder='little', signed=False);
    if args.v:
        print("Trailer:", binascii.hexlify(data[-28:]), " CRC:", hex(get_crc(data)), " Size: ", len(data))
    
    if args.o:
        file = open(args.o, 'wb')
        file.write(data);
        

