import sys
from socket import *
import zlib
import os.path

class listener:
    
    expseq = 0 # Expected packet sequence no
    
    def connect(self):
        Port = int(sys.argv[1])
        recvSocket = socket(AF_INET, SOCK_DGRAM)
        recvSocket.bind(('',Port))
        while True:
            
            # extract data from sender
            # then perform prelim checks to make sure data not corrupted
            segdata, clientAddress = recvSocket.recvfrom(2048)
            decodedata = segdata.decode()
            
            # Data should have at least 2 spaces (for checksum & seq no), otherwise it is corrupted
            if decodedata.count(' ') < 2:
                feedback = 'NAK'
                self.sendmsg(feedback, recvSocket, clientAddress)
                continue
            
            segparts = (decodedata).split(' ', 2) # Split data into checksum and segment
            
            # Checksum and seq no should consist only of digits, otherwise it is corrupted
            if (not segparts[0].isdigit()) or (not segparts[1].isdigit()):
                feedback = 'NAK'
                self.sendmsg(feedback, recvSocket, clientAddress)
                continue

            segchksum = int(segparts[0])
            recvseq = int(segparts[1])
            segment = segparts[2]

            # check whether contenthas been corrupted
            if segchksum == zlib.crc32(segment.encode()):
                feedback = 'ACK'
                if recvseq == self.expseq: # deliver packet it it is expected
                    self.expseq += 1
                    print (segment, end = '')
                    # otherwise, drop the packet as it is duplicate             
            else:
                feedback = 'NAK'
            
            # Send feedback
            self.sendmsg(feedback, recvSocket, clientAddress)
    
    def sendmsg(self, feedback, recvSocket, clientAddress):
        fbdata = feedback.encode()
        fbchksum = str(zlib.crc32(fbdata)) # checksum
        recvSocket.sendto(fbchksum.encode() + (' ').encode() + fbdata, \
        clientAddress)


def main():
    if len(sys.argv) < 2:
        raise Exception ('No connection port provided!')
    testlistener = listener()
    testlistener.connect()

if __name__ == "__main__":
    main()