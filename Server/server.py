from asyncio.windows_events import NULL # check if the file exists in the files dictionary
import binascii # import the binascii module
import socket # import the socket module
import sys # import the sys module
import threading    # import the threading module
import traceback # import the traceback module
import struct # import the struct module
import logging # import the logging module
from Crypto.Cipher import AES, PKCS1_OAEP   # import the AES and PKCS1_OAEP modules
from Crypto.PublicKey import RSA # import the RSA module
import os # import the os module
from db_handler import Database, File # import the Database and File classes
from client_handler import Client   # import the Client class
from response import Response # import the Response class

SERVER_VER = 3
DEFAULT_PORT_ADD = 1234
PORT_ADD_SOURCE = "port.info"
FILE_CHUNK_SIZE = 1024
REQ_HEADER_SIZE = 16 + 1 + 2 + 4
CLIENT_ID_SIZE = 16
VER_SIZE = 1
REQ_CODE_SIZE = 2
PAYLOAD_SIZE = 4


REGISTER = 1100
SEND_PUBLIC_KEY = 1101
RECONNECT = 1102
SEND_FILE = 1103
CRC_CORRECT = 1104
INVALID_CRC = 1105
BAD_CRC_DONE = 1106

class Server:
    # Initialize the server with a host, port, and other necessary attributes
    def __init__(self,host,port):
        self.port = port
        self.server_socket = None
        self.host = host
        self.client_threads = []
        self.db = Database()

    # Create tables in the database if necessary and run the server
    def run_server(self):
        # Create database tables and update clients dictionary
        if not self.db.create_clients_table() and not self.db.update_clients_dict():
            print("Error creating the data structure")
            return 0
        
        # Create server socket and listen for incoming connections
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Create server socket
        print(f"Server socket created \n")
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # Set socket options
        self.server_socket.bind((self.host, self.port)) # Bind socket to host and port
        self.server_socket.listen() # Listen for incoming connections
        print(f"Server version: {SERVER_VER} listening to port: {self.port} \n")
        
        while True: # Accept connections from clients and create a thread for each
            client_socket, address = self.server_socket.accept()
            print(f"Connection from {address[0]}:{address[1]}")
            client_thread = threading.Thread(target=self.handle_client, args=(client_socket,))

            client_thread.start()
            self.client_threads.append(client_thread)

    def handle_client(self, client_socket): # Handle client requests

        # Handle client requests and send appropriate responses
        try:
            while True:
                # Receive request header and unpack it
                header = client_socket.recv(REQ_HEADER_SIZE)

                cid, version, request_code_0, request_code_1, payload_size_0, payload_size_1, payload_size_2, payload_size_3 = struct.unpack("<16sB2B4B", header)
                request_code = (request_code_1 << 8) | request_code_0
                payload_size = (payload_size_3 << 24) | (payload_size_2 << 16) | (payload_size_1 << 8) | payload_size_0

                # Receive the payload based on the payload_size
                payload = client_socket.recv(payload_size)

                # Create a new client object
                client = Client(cid, None, NULL, None, NULL)

                # Check the request code and handle the request accordingly
                # If the request code is not supported, break the loop
                if client.failures > 3:
                    response = Response.handle_client_completed(client.CID)

                elif request_code == REGISTER: # Register a new client
                    name = payload.decode('ascii').strip('\0') # Get the name from the payload
                    if client.register_client(name,self.db): # Register client and send response
                        response = Response.registration_success(client.CID) # Send response
                    else:
                        response = Response.registration_failure(client.CID)

                elif request_code == RECONNECT: # Reconnect an existing client
                    name = payload.decode('utf-8')[:payload.index('\x00')+1] # Get name from payload

                    if client.client_reconnect(name,self.db): # Reconnect client and send response
                        response = Response.reconnect_approved(client.CID, client.AES) # Send response
                    else:
                        response = Response.reconnect_denied(client.CID) # Send response = connection denied

                elif request_code == SEND_PUBLIC_KEY: # Receive public key from client
                    name = payload[: -160].decode('ascii').strip('\0') # Get name from payload
                    publicKey = payload[-160:] # Get public key from payload
                    
                    if client.client_public_key_receive(name,publicKey,self.db): # Update client's public key and send response
                        print(f"Public key of {name} is {publicKey} ") # Print public key
                        aes = Server.encrypt_aes_key(publicKey) # Encrypt AES key with public key
                        self.db.update_client_aes(aes) # Update client's AES key in the database
                        response = Response.public_key_receive_aes_send(client.CID,aes) # Send response
                    else:
                        response = Response.gen_error() # Send response = error
                
                elif request_code==SEND_FILE: # Receive file from client
                    contentSize = payload[:4] # Get content size from payload
                    fileName = payload[4: 260].decode('ascii').strip('\0') # Get file name from payload
                    fileData =payload[260:] # Get file data from payload
                    decryptFileData = Server.decrypt_file(fileData,self.db.client_get_aes()) # Decrypt file data
                    recFile = File(client.Name,fileName,fileName,False) # Create a new file object
                    client.client_handle_received_file(recFile) # Handle received file
                    try:
                        Server.create_file(fileName,decryptFileData) # Create file
                        response = Response.received_File_OK_with_CRC(client.CID, contentSize,fileName,Server.rc32_checksum(fileName)) # Send response
                    except Exception as e:
                        print(f"Error processing file: {fileName}\n{str(e)}") # Print error
                        response = Response.gen_error() # Send response = error

                elif request_code==CRC_CORRECT: # Receive CRC confirmation from client
                    fileName = payload.decode('ascii').strip('\0') # Get file name from payload
                    if client.file_confirm_receive(fileName,self.db): # Confirm file reception and send response
                        response = Response.handle_client_completed(client.CID) # Send response
                    else:
                        response = Response.gen_error() # Send response = error
            
                elif request_code==INVALID_CRC: # Receive CRC error from client
                    client.failures = client.failures + 1 # Increment failures
                    
                elif request_code==BAD_CRC_DONE: # Receive CRC error confirmation from client
                    response = Response.handle_client_completed(client.CID) # Send response

                else:
                    print(f"Server does not support request {request_code}") # Print error
                    break

                client_socket.sendall(response) # Send response

        except Exception as e: # Handle exceptions
            print(f"Error handling client: {e}") # Print error
            traceback.print_exc() # Print traceback

        finally: # Close client connection
            print("Closing client connection") # Print message
            client_socket.close() # Close client socket
        
    def encrypt_aes_key(self, public_key):
        #   Generate a random AES key
        aes_key = os.random(16)

        return PKCS1_OAEP.new(RSA.import_key(public_key)).encrypt(aes_key) # Encrypt the AES key with the public key      
    
    def decrypt_file(encrypted_data, key):
        def decrypt_file(self, key): # Decrypt file data
            # Create new AES cipher
            cipher = AES.new(key, AES.MODE_EAX)

            # Extract nonce and tag from encrypted data
            nonce = encrypted_data[:cipher.nonce_size]
            tag = encrypted_data[-cipher.digest_size:]

        #  Decrypt the content and verify tag
            encrypted_content = encrypted_data[cipher.nonce_size:-cipher.digest_size] # Extract encrypted content
            decrypted_content = cipher.decrypt_and_verify(encrypted_content, tag) # Decrypt content

            return decrypted_content

        def create_file(file_name, content):
            try:
                with open(file_name, 'w') as file: # Create file available for writing content
                    file.write(content)
                return True
            except Exception:
                    return False

        def crc32_checksum(filename): # Calculate CRC32 checksum
            try:
                with open(filename, 'rb') as file: # Open file in binary mode
                    buf = file.read()
                    crc = binascii.crc32(buf) # Calculate CRC32 checksum
                    return crc & 0xFFFFFFFF # Return CRC32 checksum
            except FileNotFoundError:
                print(f"File not found: {filename}")
                sys.exit(1)
            except Exception as e:
                print(f"Error processing the file: {filename}\n{str(e)}")
                sys.exit(1)


def main():
    #  Set up logging
    try:
        with open(PORT_ADD_SOURCE, "r") as f: # Open port file 
            port = int(f.read())  # Read port from file
            print(f"port read from: {PORT_ADD_SOURCE}\n") 
    except FileNotFoundError: # Handle file not found error
        port = DEFAULT_PORT_ADD # Use default port
        logging.warning(f"Error opening file: {PORT_ADD_SOURCE} . Default port will be used: {DEFAULT_PORT_ADD} \n" )     

    server = Server('localhost',port) # Create server object
    server.run_server() # Run server 
if __name__ == '__main__':
    main()
