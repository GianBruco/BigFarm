#! /usr/bin/env python3
# echo-server.py

import sys
import struct
import socket
import select

HOST = "127.0.0.1"  # localhost
PORT = 57937  # porta di ascolto

# funzione per l'ordinamento delle coppie
def sortCoppie(elem):
    return elem[0]

# funzione che riceve n byte dal socket conn
def recv_all(conn, n):
	chunks = b''
	bytes_recd = 0
	while bytes_recd < n:
		chunk = conn.recv(min(n - bytes_recd, 1024))
		if len(chunk) == 0:
			raise RuntimeError("socket connection broken")
		chunks += chunk
		bytes_recd = bytes_recd + len(chunk)
	return chunks

# funzione per l'invio e la ricezione dei dati dai vari socket
def gestioneDati(conn):
	with conn:
		# leggo i primi 4 byte che rappresentano il numero della scelta
		data = recv_all(conn, 4)
		scelta = struct.unpack("!i", data[:4])[0]

		if not data:			# se ricevo 0 bytes la connessione è terminata
			return -1
		if scelta == 1:
			conn.sendall(struct.pack("!i", len(coppie)))
			# stampo le coppie
			for k in coppie:
				somma = str(k[0])
				conn.sendall(struct.pack("!i", len(somma)))	# mando la lunghezza della somma
				conn.sendall(somma.encode())				# mando la somma

				nome = str(k[1])
				conn.sendall(struct.pack("!i", len(nome)))	# mando la lunghezza del nome
				conn.sendall(nome.encode())					# mando il nome del file

		elif scelta == 2:
			data = recv_all(conn, 4)
			nLongs = struct.unpack("!i", data[:4])[0]		# leggo il numero di long da leggere
			for i in range(nLongs):
				
				dati = recv_all(conn, 4)
				lunghezza = struct.unpack("!i", dati[:4])[0]	# leggo la lunghezza di l
				data = recv_all(conn, lunghezza)
				num = data[:lunghezza].decode(encoding='ascii')	# leggo il long (in formato stringa)
				a = int(num)									# converto in numero
				esiste = 0
				nOut = 0
				listaout=''
				# controllo se esiste nella lista coppie
				for c in (coppie):
					if(c[0] == a):
						esiste = 1
						listaout=f'{c[0]} {c[1]}'

				# se non esiste stampo "Nessun file"
				if(esiste == 0):
					conn.sendall(struct.pack("!i", len("Nessun file")))	# mando la lunghezza di "Nessun file"
					conn.sendall("Nessun file".encode())				# mando la stringa "Nessun file"
				else:
					conn.sendall(struct.pack("!i", len(listaout)))	# mando la lunghezza di "Nessun file"
					conn.sendall(listaout.encode())					# mando la stringa "Nessun file"

		elif scelta == 3:
			
			dati = recv_all(conn, 4)
			lunghezzaS = struct.unpack("!i", dati[:4])[0]			# leggo la lunghezza di somma
			data = recv_all(conn, lunghezzaS)
			sommaCoppia = data[:lunghezzaS].decode(encoding='ascii')# leggo la stringa somma
			sommaCoppia = int(sommaCoppia)
			
			dati = recv_all(conn, 4)
			lunghezzaF = struct.unpack("!i", dati[:4])[0]			# leggo la lunghezza del nome file
			data = recv_all(conn, lunghezzaF)
			nomeCoppia = data[:lunghezzaF].decode(encoding='ascii')	# leggo la stringa nome
			esiste=0
			for j in (coppie):
				if(j[0]==sommaCoppia) and (j[1]==nomeCoppia):
					esiste=1
			if(esiste==0):
				coppie.append([sommaCoppia, nomeCoppia])
				coppie.sort(key=sortCoppie)


# main
coppie = []
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
	try:
		s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		s.bind((HOST, PORT))
		s.listen()
		inputsk = [s]
		while len(inputsk) > 0:
			inready, _, _ = select.select(inputsk, [], [], 10)		# controllo i dati ogni 10 secondi
			if len(inready) == 0:
				print("Non è arrivato nulla per ora")
			else:
				# per ogni socket con dati pronti
				for pronti in inready:
					if pronti is s:
						conn, addr = s.accept()
						print(f"Mi ha contattato {addr}")
						inputsk.append(conn)
					else:
						inputsk.remove(pronti)
						gestioneDati(pronti)
						
		print("La lista di socket in input è vuota")
	except KeyboardInterrupt:
		print('Fine dei giochi!')

	# spengo il server
	s.shutdown(socket.SHUT_RDWR)
