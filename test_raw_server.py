#!/usr/bin/python3

import socket

DstIP     = "127.0.0.1"
SrcIP     = "127.0.0.1"

DstPort   = 56000
SrcPort   = 55000

bufferSize  = 1024

msgFromServer       = "Hello UDP Client"

bytesToSend         = str.encode(msgFromServer)

# Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)

UDPServerSocket.bind((SrcIP,SrcPort));

print("UDP server up")

for i in range(100):

    UDPServerSocket.sendto(bytesToSend, (DstIP,DstPort))
