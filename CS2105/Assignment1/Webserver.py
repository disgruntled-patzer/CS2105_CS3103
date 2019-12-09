# CS2105 Assignment 1: Webserver.py
# Author: Lau Yan Han, e0148792@u.nus.edu

import sys
from socket import *
import os.path #To check whether file exists

class HttpRequest:
    
    __Request = bytearray()
    __RequestType, __path, __ContentBody = "", "", ""
    __ContentLength = 0

    # get the single http request
    def __init__(self, clientinput):
        self.Request = clientinput
    
    # split the single request into respective commands
    def ParseHttpRequest(self):
        
        Commands = self.Request.split()
        self.__RequestType = Commands[0].upper().decode() # Request Type not case sensitive!
        self.__path = Commands[1].decode()
        if (self.__RequestType == 'POST'): # For POST commands where content length/body is included
            self.__ContentBody = Commands[-1] # content body can be in raw bytes so don't decode
            self.__ContentLength = Commands[-2].decode()
    
    def GetRequestType(self):
        return self.__RequestType
    
    def GetPath(self):
        return self.__path

    def GetContentLength(self):
        return self.__ContentLength
    
    def GetContentBody(self):
        return self.__ContentBody

class Webserver:

    # in: Client input, may or may not contain several requests; out = response to client
    # SingleRequest = contains a single request which will be used by HandleClientRequest()
    buffer_in, buffer_out, SingleRequest = bytearray(), bytearray(), bytearray()
    ParsingContent = False # detect whether extractrequests() is parsing through header or content
    hashtable = dict() # declare a dictionary (hashtable) in memory for key-value access
    
    def start(self):
        serverPort = int(sys.argv[1])
        listeningSocket = socket(AF_INET, SOCK_STREAM)
        listeningSocket.bind(('', serverPort))
        listeningSocket.listen(1)
        print ('The server is ready to receive')
        while True:
            connectionSocket, addr = listeningSocket.accept()
            print ('Connected to a client, awaiting incoming requests.')
            
            # buffer_in will continually extract data from the client input
            # and call extractRequests() to extract single requests (if any) for processing
            # When buffer_in extracts zero data, then there is no more client input
            # and the loop will terminate after closing the connectino socket
            self.buffer_in = connectionSocket.recv(1024)
            while len(self.buffer_in) > 0:
                self.extractRequests(connectionSocket)
                self.buffer_in = connectionSocket.recv(1024)
            # connectionSocket.close()
            # print("Connection closed")
        listeningSocket.close()

    # Parse through buffer_in to extract a SINGLE http request
    def extractRequests(self, connectionSocket):
        
        # We could use a simple python for loop, but I want to access the previous element...
        for i in range(len(self.buffer_in)):
            
            self.SingleRequest.append(self.buffer_in[i]) # Fill up the Single Request

            space = ord(' ') # get ASCII value of whitespace for byte conversion
            # End of header is reached when double space is detected
            if (self.buffer_in[i] == space) and (self.buffer_in[i - 1] == space):
                
                # For the parsing to continue, the Single Request must start with POST
                # (since only POST requests come with content body)
                if (self.SingleRequest.startswith (('POST').encode()) ):
                    self.ParsingContent = True # i has left the header, and start parsing content
                    # Get the content lengthy. Note that:
                    #   1. Last part of header (before double whitespace) contains content length
                    #   2. Header is originally in ASCII bytes: Convert to char number, then to int
                    ContentLen = int(chr(self.SingleRequest[-3]))
                    count = 0 # To track content length later
                else: # i has reached end of request; send out the request before resetting it
                    self.handleClientRequest(connectionSocket)
                    self.SingleRequest = bytearray() # Reset Single Request to accept next request

            # If request is POST and i is parsing through content body
            if self.ParsingContent == True:
                # End of content body is reached when expected length is reached
                if count == ContentLen:
                    self.handleClientRequest(connectionSocket)
                    self.SingleRequest = bytearray()
                    self.ParsingContent = False # Reset this bool value for next request
                count = count + 1 # increment count by 1 for next parse through contentbody

        # note that if buffer_in does not form a complete single request
        # then this function just exits with a half-completed SingleRequest
        # and waits for information from the next copy of buffer_in

    # Handle a SINGLE request and form response accordingly
    def handleClientRequest(self, connectionSocket):
        
        # For each request, intialize a HttpRequest object to parse the request
        # and return the necessary info (RequestType and path)
        clientrequest = HttpRequest(self.SingleRequest)
        clientrequest.ParseHttpRequest()

        RequestType = clientrequest.GetRequestType()
        path = clientrequest.GetPath()

        # Make sure request is valid
        if RequestType != 'GET' and RequestType != 'POST'\
            and RequestType != 'DELETE':
            print ("Error: Request type can only be GET, POST or DELETE")
            self.sendBadRequest(connectionSocket)
            return
        
        # Check whether it is static file serving (/file/)
        # Or key-value access (/key/)
        if path.startswith('/file/'):
            if RequestType != 'GET':
                print ("Error: /file/ can only be used in GET commands")
                self.sendBadRequest(connectionSocket)
            else:
                ResponseLength = self.serveStaticFile(path) # open and read file
                if not self.buffer_out: # if file not found
                    print("Error: File not found")
                    self.sendNotFound(connectionSocket)
                else:
                    self.sendOKResponse(connectionSocket, ResponseLength)
        
        elif path.startswith('/key/'):
            
            key = path.replace('/key/','') # Get key-value commands from ContentBody
            value = clientrequest.GetContentBody()
            
            if RequestType == 'GET' or RequestType == 'DELETE':
                # Both GET and DELETE need to retrieve data
                ResponseLength = self.retrieveKey(key)
                if ResponseLength == -1:
                    print ("Error: Specified key does not exist")
                    self.sendNotFound(connectionSocket)
                else:
                    if RequestType == 'DELETE': # Only DELETE needs to delete the data
                        del self.hashtable[key]
                    self.sendOKResponse(connectionSocket, ResponseLength)
            
            else: # RequestType == 'POST'
                self.hashtable[key] = value
                self.sendOKResponse(connectionSocket, -1)

        else:
            print ("Error: Path prefix must be either /file/ or /key/")
            self.sendBadRequest(connectionSocket)
    
    # Static file serving. Function will modify the buffer_out bytearray
    # to contain the data from served file (empty if file not found)
    # It returns the size of the file (ResponseLength); 0 if file not found
    def serveStaticFile(self, path):
        filename = path.replace('/file/','') #Remove the /file/ command
        
        # First make sure the file exists (this has a caveat where
        # the file may be moved after this check is performed and
        # before the file is opened. But we can assume such a thing
        # won't happen in this assignment...)
        if (not os.path.isfile(filename)):
            return 0
        
        # Open the file, scan it and dump it into buffer_out
        # readinto will read and dump data from file into bytereader
        # until bytereader is full; numbytesread tracks no. of bytes
        # in bytereader. bytereader then dumps data into buffer_out
        # and the process repeats to fill up bytereader again
        with open (filename, "rb") as filereader:
            bytereader = bytearray(1024)
            numbytesread = filereader.readinto(bytereader)
            ResponseLength = numbytesread
            while numbytesread > 0:
                self.buffer_out += bytereader # .append() doesn't work; only works for int o.o
                numbytesread = filereader.readinto(bytereader)
                ResponseLength += numbytesread
            # end of file when bytereader has 0 bytes: No more data to read
        
        return ResponseLength # Return the content length of the file

    # Retrieve a key from the key-value dictionary, and add it to output buffer
    # Return length of the value, or -1 if key-value doesn't exist
    def retrieveKey(self, key):
        if key in self.hashtable:
            self.buffer_out += self.hashtable[key]
            return len(self.hashtable[key])
        else:
            return -1
    
    def sendBadRequest(self, connectionSocket):
        print ('Sending Response: 400 BadRequest  ')
        connectionSocket.send(('400 BadRequest  ').encode())
        self.buffer_out = bytearray() # clear output buffer after sending it

    def sendNotFound(self, connectionSocket):
        print ('Sending Response: 404 NotFound  ')
        connectionSocket.send(('404 NotFound  ').encode())
        self.buffer_out = bytearray() # clear output buffer after sending it

    def sendOKResponse(self, connectionSocket, ResponseLength):
        # ResponseLength = -1 means there is no content body to be sent
        if ResponseLength == -1:
            response = ('200 OK  ').encode()
        else:
            # Note; Encode everything into bytes
            response = '200 OK content-length '.encode() + str(ResponseLength).encode() \
            + ('  ').encode() + (self.buffer_out)
        print ('Sending Response:', response)
        connectionSocket.send(response) # then the whole thing encoded back and sent out
        self.buffer_out = bytearray() # clear output buffer after sending it


def main():
    if len(sys.argv) < 2:
        raise Exception ('No listening port provided!')
    testserver = Webserver()
    testserver.start()

if __name__ == "__main__":
    main()