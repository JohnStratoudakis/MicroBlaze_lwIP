#!C:\Users\johns\AppData\Local\Programs\Python\Python37-32\python.exe

# This script will upload the specified elf file to a remote server using SFTP
# Then it will connect to that machine and run the patchBitxfile.py script
# which will update the lvbitx file.

elfPath = ".\\"
elfFile = 'mb_lwip.elf'
#targetDir = "/home/john/work/git/fpganow/MicroBlaze_lwIP/xilinx_mb_lwip/mb_lwip/Release/"
targetDir = "/home/john/builds/labview_fpga_nic/FPGATester/"
privateKeyFile = "..\\..\\..\\deploy"
updateScirpt = "/home/john/builds/labview_fpga_nic/FPGATester/update.sh"

import os
import sys

def getHost():
    import socket
    host = socket.gethostbyname(socket.gethostname())
    if ".0." in host:
        print("Detected apt")
        return "192.168.0.14"
    else:
        print("Detected flushing")
        return "apt.johnstratoudakis.com"

targetHost = getHost()

elfFullPath = elfPath + elfFile
print("Post-Build action")

print("Checking if private key file exists")
if os.path.isfile(privateKeyFile):
    print(f'Private key file {privateKeyFile} exists')
else:
    print(f'Could not find private key file {privateKeyFile}')
    print('Exiting...')
    sys.exit(1)

print("Checking if elf file exists")
if os.path.isfile(elfFullPath):
    print(f'Elf file {elfFullPath} exists')
else:
    print(f'Could not find elf file {elfFullPath}')
    print('Exiting...')
    sys.exit(1)


import pysftp
cnopts = pysftp.CnOpts()
cnopts.hostkeys = None

with pysftp.Connection(targetHost, username='john', private_key=privateKeyFile, cnopts=cnopts) as sftp:
    print("Successfully connected")
    sftp.put(elfFullPath, targetDir + elfFile)

from fabric import Connection
conn = Connection(host=targetHost, user='john',
                    connect_kwargs={'key_filename': privateKeyFile})
result = conn.run('uname -s', hide=True)
print(f'result = {result.stdout}')

res = conn.run('cd /home/john/builds/labview_fpga_nic/FPGATester && ~/bin/patchBitxfile.py --bit PXIe6592R_Top_Gen2x8.bit --lvbitx fpga_nic.lvbitx --no-confirmation')
print(f'res = {res.stdout}')
