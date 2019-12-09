# Useful Link: https://github.com/xhacker/RDT-over-UDP

import sys
from socket import *
import zlib
import os.path

SEG_SIZE = 45 # Max size is 64 bytes. Allocate 19 bytes to the checksum + seq number

class sender:

    # All "global" variables are stored here (to avoid passing them among functions)
    seq = 0 # sequence number
    serverPort = 0
    serverName = ''
    fulldata = bytearray()
    feedback = ''
    spc = (' ').encode() # encoded space character
    
    def connect(self):
        
        self.serverName = 'lenovo-Lenovo-G560' #to be replaced with sunfire host name
        self.serverPort = int(sys.argv[1])
        sendingSocket = socket(AF_INET, SOCK_DGRAM)
        
        while True:
            
            message = input('Enter Message: ')
            offset = 0
            
            # Parse through message to break it down into smaller segments if necessary
            while offset < len(message):
                if offset + SEG_SIZE < len(message): # if remaining part of message larger than segment size
                    segment = message[offset:offset + SEG_SIZE] # extract a segment size of the message
                else: # if remaining part of message can fit into segment size
                    segment = message[offset:] # extract the whole message
                offset += SEG_SIZE # then move offset forward for next parse
            
                # Add checksum to segment and send it to receiver
                segdata = segment.encode()
                if offset > len(message): # Add newline if end of message reached
                    segdata += ('\n').encode()
                segchksum = str(zlib.crc32(segdata)) # checksum
                self.fulldata = segchksum.encode() + self.spc + str(self.seq).encode()\
                     + self.spc + segdata
                sendingSocket.sendto(self.fulldata, (self.serverName, self.serverPort))
                self.feedback, serverAddress = sendingSocket.recvfrom(2048)
                self.verifyreception(sendingSocket)

                self.seq += 1 # increment packet seq no by 1
    
    # verify that receipient has not received corrupted packet
    # if packet was corrupted, resend
    def verifyreception(self, sendingSocket):
        
        while True:
            decodefbk = self.feedback.decode()
            print (decodefbk)
            
            # Data should have at least 1 space (for checksum), otherwise it is corrupted
            if decodefbk.count(' ') < 1:
                self.resendmsg(sendingSocket)
                continue
            
            fbkparts = (decodefbk).split(' ', 1)

            # insert check to make sure checksum consist of digits
            if not fbkparts[0].isdigit():
                self.resendmsg(sendingSocket)
                continue

            fbkchecksum = int(fbkparts[0])
            fbksegment = fbkparts[1]
            
            if not fbkchecksum == zlib.crc32(fbksegment.encode()):
                self.resendmsg(sendingSocket)
                continue

            if fbkparts[1] == 'ACK':
                break
            else:
               self.resendmsg(sendingSocket)

    def resendmsg(self, sendingSocket):
        sendingSocket.sendto(self.fulldata, (self.serverName, self.serverPort))
        self.feedback, serverAddress = sendingSocket.recvfrom(2048)

def main():
    if len(sys.argv) < 2:
        raise Exception ('No connection port provided!')
    testsender = sender()
    testsender.connect()

if __name__ == "__main__":
    main()