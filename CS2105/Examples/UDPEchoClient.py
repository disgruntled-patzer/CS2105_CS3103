from socket import *

serverName = 'sunfire.comp.nus.edu.sg'
serverPort = 12000
clientSocket = socket(AF_INET, SOCK_DGRAM)
message = input('Input lowercase sentence: ')
clientSocket.sendto(message.encode(), (serverName, serverPort))
modifiedMessage, addr = clientSocket.recvfrom(2048)
print(modifiedMessage.decode(), addr)
clientSocket.close()

