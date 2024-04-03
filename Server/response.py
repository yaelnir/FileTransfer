# response.py
import struct

class Response:
    VERSION = 3

    REGISTER_SUCCESS = 2100
    REGISTER_FAILURE = 2101
    RECEIVED_PUBLIC_KEY_SEND_AES = 2102
    FILE_RECEIVE_OK_WITH_CRC = 2103
    RECEIVED_FILE_CONFIRM = 2104
    RECONNECT_APPROVE = 2105
    RECONNECT_DENIED = 2106
    ERROR_GENERAL = 2107

    CLIENT_ID_SIZE = 16
    AES_SIZE = 128
    CONTENT_SIZE = 4
    FILE_NAME_SIZE = 255
    CKSUM_SIZE = 4

    # Builds the header for the response
    @staticmethod 
    def build_header(response_code, payload_size):
        return struct.pack("<B H I", Response.VERSION, response_code, payload_size) # pack the header

    @staticmethod
    def registration_success(client_id): 
        response_code = Response.REGISTER_SUCCESS
        payload_size = Response.CLIENT_ID_SIZE # size of the payload
        header = Response.build_header(response_code, payload_size) # build the header
        print(f"Response: {response_code}, successful registration")
        return header + client_id

    @staticmethod
    def registration_failure(client_id):
        response_code = Response.REGISTER_FAILURE
        payload_size = 0
        header = Response.build_header(response_code, payload_size)
        print(f"Response: {response_code} Registration Failed")
        return header

    @staticmethod
    def public_key_receive_aes_send(client_id, encrypted_aes_key): # send the public key to the client
        response_code = Response.RECEIVED_PUBLIC_KEY_SEND_AES # response code
        payload_size = Response.CLIENT_ID_SIZE + Response.AES_SIZE 
        header = Response.build_header(response_code, payload_size) 
        return header + client_id + encrypted_aes_key # return the header and the payload
    
    @staticmethod
    def reconnect_approved(client_id, encrypted_aes_key):
        response_code = Response.RECONNECT_APPROVE
        payload_size = Response.CLIENT_ID_SIZE + Response.AES_SIZE
        header = Response.build_header(response_code, payload_size)
        encrypted_aes_key_bytes = encrypted_aes_key.to_bytes(16, byteorder='little')

        print(f"Response:  {response_code} Reconnect approved")
        return header + client_id + encrypted_aes_key_bytes
        
    @staticmethod
    def reconnect_denied(client_id):
        response_code = Response.RECONNECT_DENIED
        payload_size = Response.CLIENT_ID_SIZE
        header = Response.build_header(response_code, payload_size)
        print(f"Response: {response_code}, Reconnect denied")
        return header + client_id

    @staticmethod
    def public_key_receive_aes_send(client_id, encrypted_aes_key):
        response_code = Response.RECEIVED_PUBLIC_KEY_SEND_AES
        payload_size = Response.CLIENT_ID_SIZE + Response.AES_SIZE
        header = Response.build_header(response_code, payload_size)
        print(f"Response: {response_code}, received Public key, sending AES")
        return header + client_id + encrypted_aes_key

    @staticmethod
    def received_File_OK_with_CRC(client_id, contentSize,fileName,cksum):
        response_code = Response.FILE_RECEIVE_OK_WITH_CRC 
        payload_size = Response.CLIENT_ID_SIZE + Response.CONTENT_SIZE +Response.FILE_NAME_SIZE + Response.CKSUM_SIZE
        header = Response.build_header(response_code, payload_size)
        print(f"Response: {response_code}, received file OK with CRC")
        return header + client_id + contentSize + fileName + cksum

    @staticmethod
    def handle_client_completed(client_id):
        response_code = Response.RECEIVED_FILE_CONFIRM
        payload_size = Response.CLIENT_ID_SIZE
        header = Response.build_header(response_code, payload_size)
        print(f"Response: {response_code}, file received confirmation")
        return header + client_id

    @staticmethod
    def gen_error():
        response_code = Response.ERROR_GENERAL
        payload_size = 0
        header = Response.build_header(response_code, payload_size)
        print(f"Response: {response_code} General error")
        return header