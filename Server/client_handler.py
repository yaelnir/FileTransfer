import sqlite3
import datetime
import db_handler
import uuid
from datetime import datetime

class Client: 
    def __init__(self, CID, Name, PublicKey, LastSeen, AES):
        self.CID = CID
        self.Name = Name
        self.PublicKey = PublicKey
        self.LastSeen = LastSeen
        self.AES = AES
        self.failures = 0;

    # Prints the client information    
    def print_client_information(self):
        print(f"Client id: {self.CID}")
        print(f"Client name: {self.Name}")
        print(f"Client public key: {self.PublicKey}")
        print(f"Client last seen: {self.LastSeen}")
        print(f"Client AES key: {self.AES}")

    # Handles registration request
    def register_client(self,name,db_instance):
        print(f"registration request from {name}")
        self.Name = name
        # Verify if the client exists in the database
        if db_instance.client_exists_by_name(self.Name): 
            print(f"Client: {self.Name} exists")
            return False
        new_cid = uuid.uuid4().bytes # generate a new client id
        self.CID = new_cid # set the client id
        self.LastSeen = datetime.now().strftime('%Y-%m-%d %H:%M:%S') # get the current date and time
        
        # Add the client to the database
        db_instance.add_client(self)
        self.print_client_information() # print client information
        return True
    # Handles a request to reconnect
    def client_reconnect(self,name,db_instance):
        print(f"Reconnect request from: {name}")
        self.Name = name
        # Verify if the client exists in the database
        if db_instance.client_exists_by_name(self.Name):
            print(f"Client: {self.Name} exists")
            return True
        return False

    # Handles a request to send the public key to the server
    def client_public_key_receive(self,name,public_key,db_instance):
        print(f"Request to send public key to server from: {name}")
        self.Name = name
        if db_instance.client_exists_by_name(self.Name): # Verify if client exists in database
            print(f"Client: {self.Name} exists")
            return db_instance.update_client_public_key(self,public_key) # Update the client's public key
        print(f"Client: {name} was not found, cannot receive public key")
        return False
    # Handles a request to send the AES key to the server
    def client_handle_received_file(self,file,db_instance): 
        print(f"Request to send: {file.fileName} to server")
        db_instance.add_file(file) # Add the file to the database
    # Handles a request to send the AES key to the server
    def file_confirm_receive(self,fileName,db_instance):
        return db_instance.update_file_verified(fileName,True) # Update the file's verified value