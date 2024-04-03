//#ifndef CLIENT_H
//#define CLIENT_H
#include <boost/asio.hpp>
using namespace boost::asio::ip;
#include <boost/crc.hpp>
#include <cstddef>
#include <iostream> 
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#include <cstdint>
#include <sstream>
#include <iomanip>
#include <bitset>

#include <aes.h>
#include <modes.h>
#include <rsa.h>
#include <cryptlib.h>

#include <osrng.h>
#include <files.h>
#include <crc.h>
#include <base64.h>

#include <filters.h>
#include <sha.h>
#include <oaep.h>

using namespace CryptoPP;
using namespace std;
using boost::asio::ip::tcp;



// structure of a server response
struct Response {
    uint8_t server_version; // 1 byte, server version
    uint16_t response_code; // 2 bytes, response code
    uint32_t payload_size; // 4 bytes, payload size
    std::vector<uint8_t> payload; // variable size, response payload
    // Constructor for the Response struct
    Response(uint16_t response_code, const std::vector<uint8_t>& payload)
        : server_version(3), response_code(response_code), payload_size(payload.size()), payload(payload) {
    }
};
// structure of a client request
struct Request {
    std::array<uint8_t, 16> cid; // 16 bytes, client ID
    uint8_t version = 3; // 1 byte, server version
    std::array<uint8_t, 2> request_code; // 2 bytes, request code
    uint32_t payload_size; // 4 bytes
    std::vector<uint8_t> payload; // variable size, request payload
};


class Client {
public:
        boost::asio::ip::tcp::socket socket_;
       // Client() = default;

        // Constructor for the Client class
        
        // the Client class receives: io_context, host, port, user and pass
        
        Client(boost::asio::io_context& io_context, const std::string& host, int port, const std::string& user, const std::string& pass);
 //       Client(boost::asio::io_context& io_context, const std::string& ip, int port, const std::string& client_name, const std::string& file_path);
        


    void connectToServer(); // declare the connectToServer function defined @@@
    void clientSaveName(const std::string &client_name); // declare the clientSaveName function defined @@@
    void clientSaveID(const std::vector<uint8_t> &data);
    void clientRegister(); // declare the clientRegister function  defined @@@
    Request reg_req; // declare the reg_req object
    void clientReconnect(); // declare the clientReconnect function  defined @@@
    void generateKeysRSA(); // declare the generateKeysRSA function defined @@@
    void publicKeySend(const CryptoPP::RSA::PublicKey& public_key); // send public key to server
    void sendRequest(const std::vector<uint8_t> &request); // send request to server declaration defined @@@
    void sendRequest(const std::string &request); // send request to server declaration defined @@@
    Response receiveResponse(); // receive response from server declaration defined @@@
    void printServerResponse(const Response &response); // print server response declaration defined @@@
    void disconnectFromServer(); // disconnect from server declaration defined @@@
    bool aESKeyDecrypt(); // AES key decryption declaration
    std::vector<uint8_t> encryptFile();
    void cRCInvalidRetry();
    void cRCInvalidDone(); // CRC invalid done declaration defined @@@
    // file encryption declaration
    void fileSend(); // file send declaration
    void cRCvalidate(); // CRC validation declaration
    void privateKeySave(const CryptoPP::RSA::PrivateKey& private_key_); // private key save declaration
    void clientNameRead(); // client name read declaration
    void clientIDRead(); // client ID read declaration
    void clientPrivateKeyRead();
    bool fileExists(const std::string &file_path); // file exists declaration
    // Appends the private key (in Base64 format) to the "me.info" file
    void run(); // start the client

private:
    static boost::asio::io_context default_io_context_;
    boost::asio::io_service io_service_;
    boost::asio::ip::tcp::resolver resolver_;
    static boost::asio::ip::tcp::socket default_socket_;
    boost::asio::ip::tcp::endpoint endpoint_;
        
    boost::asio::io_context& io_context_;
    
    CryptoPP::RSA::PrivateKey private_key_; // private key declaration
    int error_count_ = 0;

    // Member variables for user and pass
    std::string user_;
    std::string pass_;

    // Member variables for client name and file path
    std::string client_name_;
    std::string file_path_;
    
};

//#endif // CLIENT_H
