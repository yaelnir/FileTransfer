import sqlite3
import threading
import client_handler
from datetime import datetime

SQL_FILE_NAME = "server.db"

class Database:

    def __init__(self):
        # connect to the database
        self.conn = sqlite3.connect(SQL_FILE_NAME, check_same_thread=False)
        self.cursor = self.conn.cursor()
        #  create the clients table
        self.files = {}
        self.clients =  {}
        self.file_lock = threading.Lock()
        
    def create_clients_table(self):
        #  Verify clients tables exist
        self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='clients'")
        result = self.cursor.fetchone()
        if not result:
            # Create clients table
            self.cursor.execute('''CREATE TABLE clients
                    (ID BLOB(16) PRIMARY KEY,
                    Name TEXT(255) NOT NULL,
                    PublicKey BLOB(160) NOT NULL,
                    LastSeen DATETIME NOT NULL,
                    AesKey BLOB(16) NOT NULL)''')
            print("Clients table created")
            return True
        print("Client table exists, update dictionary")
        return False
        
    def create_files_table(self):    
        #  Verify files table exists
        self.cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='files'")
        result = self.cursor.fetchone()
        if not result:
            # Create files table
            self.cursor.execute('''CREATE TABLE files
                        (ID BLOB(16) PRIMARY KEY,
                        FileName TEXT NOT NULL,
                        PathName TEXT NOT NULL,
                        Verified BOOLEAN NOT NULL)''')
            print("Files table created")
            return True
        print("Files table exists, updates dictionary")
        return False
        
    
    def update_clients_dict(self):
        with self.file_lock:
            try:
                self.cursor.execute("SELECT * FROM clients")
                rows = self.cursor.fetchall()
                for row in rows:
                    client = client_handler.Client(row[0], row[1], row[2], row[3], row[4])
                    self.clients[client.CID] = client
                print("The client dictionary has been updated with the existing data")
                return True
            except Exception:
                return False

    def update_files_dictionary(self): # update the files dictionary with the existing data in the files table
        with self.file_lock: # lock the file to prevent multiple threads from accessing the file at the same time
            try:
                self.cursor.execute("SELECT * FROM files")
                rows = self.cursor.fetchall()
                # iterate through the rows and create a file object for each row
                for row in rows:
                    file = File(row[0], row[1], row[2], row[3])
                    self.files[file.ID] = file
                print("Files dictionary updated")
                return True
            except Exception:
                return False


    def add_client(self, client): # add a client to the clients table in the database
        self.cursor.execute("INSERT INTO clients (ID, Name, PublicKey, LastSeen, AesKey) \
                        VALUES (?, ?, ?, ?, ?)",
                        (client.CID, client.Name, client.PublicKey, client.LastSeen, client.AES))
        self.conn.commit() # commit the changes to the database
        self.clients[client.CID] = client # add the client to the clients dictionary
        print(f"Client: {client.Name} registered successfully.")
    
    
    def find_client_by_id(self, client):
        # Verify if client exists in clients table per client id
        self.cursor.execute("SELECT ID FROM clients WHERE ID=?" , (client.CID))
        result = self.cursor.fetchone()
        if result and client.Name in self.clients:
            return True
        else:
            return False

    def find_file_by_id(self, file):
        #  Verify if file exists in files table in database per file id
        self.cursor.execute("SELECT id FROM files WHERE id=?", (file.ID,))
        result = self.cursor.fetchone()
        if result and file.ID in self.files: # check if the file exists in the files dictionary
            return True
        else:
            return False

    def find_file_by_name(self, fileName):
        # Verify if file exists in files table per file name
        self.cursor.execute("SELECT FileName FROM files WHERE FileName=?", (fileName,))
        result = self.cursor.fetchone()
        if result:
            return True
        else:
            return False
        
    def find_client_by_name(self, name):
        # Verify if a client exists in the clients table per client name
        self.cursor.execute("SELECT Name FROM clients WHERE Name=?", (name,))
        result = self.cursor.fetchone()
        if result:
            return True
        else:
            return False


    def update_client_aes(self, client):
        try:
            # update a client's AES in the clients table 
            self.cursor.execute("UPDATE clients SET aes=? WHERE id=?", (client.AES, client.CID))
            self.conn.commit()

            #  update a client's AES in the clients dictionary
            if client.CID in self.clients:
                self.clients[client.CID].AES = client.AES # update the AES value in the dictionary
            print("AES updated")
            return True
        except Exception as e:
            print("Error while updating AES:", e)
            return False
    

    def update_client_public_key(self, client,public_key):
        try:
            #  update a client's public key in the clients table
            self.cursor.execute("UPDATE clients SET PublicKey=? WHERE id=?", (public_key, client.CID))
            self.conn.commit()

            # update a client's public key in the clients dictionary
            if client.CID in self.clients:
                self.clients[client.CID].public_key = public_key

            print("public key updated successfully!")
            return True

        except Exception as e:
            print("Error updating public key:", e)
            return False
    
    def add_file(self, file):
        #  add file to the files table 
        self.cursor.execute("INSERT INTO files (id, file_name, path_name, verified) VALUES (?, ?, ?, ?)", 
                            (file.ID, file.file_name, file.path_name, file.verified))
        self.conn.commit() # commit the changes to the database

        self.files[file.ID] = file # add the file to the files dictionary
    
    def client_update_last_seen(self, client):
        last_seen = datetime.now().strftime('%Y-%m-%d %H:%M:%S') # get the current date and time
        # update client's last seen value in the clients table
        self.cursor.execute("UPDATE clients SET LastSeen = ? WHERE id = ?", (last_seen, client.CID))
        self.conn.commit() # commit the changes to the database

        # update client's last seen in dictionary
        self.clients[client.CID].LastSeen  = last_seen

    # update the verified value of a file in the files table
    def file_verified_update(self, file_id, verified):
        try:
            #  update verified value in files table
            self.cursor.execute("UPDATE files SET verified=? WHERE id=?", (verified, file_id))
            self.conn.commit()

            # update verified in files dictionary
            if file_id in self.files:
                self.files[file_id].verified = verified
            print("Verified value updated")
            return True

        except Exception as e:
            print("Error while updating verified value:", e)
            return False

    def client_get_aes(self, client): # get client's AES key from clients dictionary
        if client.CID in self.clients:
            return self.clients[client.CID].AES
        else:
            return None

    def client_get_public_key(self, client): # get client's public key from clients dictionary
        if client.CID in self.clients:
            return self.clients[client.CID].public_key
        else:
            return None
# File class
class File:
    def __init__(self, ID, file_name, path_name, verified):
        self.ID = ID
        self.file_name = file_name # file name
        self.path_name = path_name # path name
        self.verified = verified # verified value
    
    def client_by_id(self, CID):
        self.clients.get(CID) # get the client by ID
