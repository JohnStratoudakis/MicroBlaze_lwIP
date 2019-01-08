
# This script is intended to be run from the PXIe chassis
# It will download the latest version of 'FPGATester.exe' from 
# an SFTP server using the provided username and key

# Create Target Directory
privateKeyFile='C:\\Users\\John\\Desktop\\pxi_key'
targetdir = 'C:\\Users\\John\\Desktop\\FPGATester\\'

sftpHost = '192.168.0.14'
sftpUser = 'john'
sftpRemoteDir = '/home/john/builds/labview_fpga_nic/FPGATester/'

import os
import shutil

print("Automating PXIe Tasks")

if os.path.isdir(targetdir):
	shutil.rmtree(targetdir)
os.mkdir(targetdir)

import pysftp
cnopts = pysftp.CnOpts()
cnopts.hostkeys = None

with pysftp.Connection(sftpHost, username=sftpUser, private_key=privateKeyFile, cnopts=cnopts) as sftp:
    print("Successfully connected")
    sftp.get_d(sftpRemoteDir,targetdir)

from subprocess import Popen
p1 = Popen(targetdir + 'FPGATester.exe' )
p1.wait()

with pysftp.Connection(sftpHost, username=sftpUser, private_key=privateKeyFile, cnopts=cnopts) as sftp:
    print(f"Successfully connected to {sftpHost} as {sftpUser}")
    from os import listdir
    from os.path import isfile, join
    onlyfiles = [f for f in listdir(targetdir) if isfile(join(targetdir, f)) and f.endswith('.log')]
    for file in onlyfiles:
        print("Sending log file: {}".format( targetdir + file ))
        sftp.put(targetdir + file, sftpRemoteDir + file)
