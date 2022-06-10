#! /usr/bin/env python3
# echo-server.py

import socket

HOST = "127.0.0.1"  # localhost
PORT = 57937  # porta di ascolto

# creazione del server
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    while True:
      print("Aspetto il client!")
      # mi metto in attesa di una connessione
      conn, addr = s.accept()
      # lavoro con la connessione appena ricevuta 
      with conn:  
        print(f"Sono stato contattato con {addr}")
        while True:
            data = conn.recv(64) # leggo fino a 64 bytes
            print(f"Ho riceuto {len(data)} bytes") 
            if not data:           # se ricevo 0 bytes 
                break              # la connessione è terminata
            conn.sendall(data)     # altrimenti echo
        print("Connessione terminata!")





