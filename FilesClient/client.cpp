#include "client.h"
#include <boost/asio.hpp>
#include <boost/asio/buffered_read_stream.hpp>
#include <cstring>
#include <iostream>
#include <cstdint> 
#include <array> 
#include <vector>
#include <boost/coroutine/detail/parameters.hpp>
#include <string>
#include <aes.h>
#include <stdexcept>

// Constructor for the Client class
boost::asio::io_context Client::default_io_context_;
boost::asio::ip::tcp::socket Client::default_socket_(Client::default_io_context_);
boost::asio::io_context io_context;
boost::asio::ip::tcp::resolver resolver(io_context);
std::string ip = "127.0.0.1";
int port = 8000;
std::string client_name = "client1";
std::string file_path = "/path/to/file";
Client client(io_context, ip, port, client_name, file_path);
// error in the following line
//Client::Client() : io_context_(default_io_context_), socket_(io_context_) {}

Client::Client(boost::asio::io_context& io_context, const std::string& host, int port, const std::string& user, const std::string& pass)
    : io_context_(io_context), socket_(io_context), user_(user), pass_(pass) {
    clientRegister();
    // Resolve the server address
    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(tcp::v4(), host, std::to_string(port));
    // Connect to the server
    boost::asio::connect(socket_, endpoints);
}

//Client::Client(boost::asio::io_context& io_context, const std::string& ip, int port, const std::string& client_name, const std::string& file_path)
  //  : io_context_(io_context), socket_(io_context), client_name_(client_name), file_path_(file_path) {
    // Resolve the server address
   // tcp::resolver resolver(io_context_);
   // auto endpoints = resolver.resolve(tcp::v4(), ip, std::to_string(port));
    // Connect to the server
   // boost::asio::connect(socket_, endpoints);
//}


    
// Main function that runs the client
void Client::run() {
    
    // Try connecting to the server
    void connectToServer();
    // If "me.info" is exist, try to reconnect to server
    void clientReconnect();
    void clientRegister();

    bool fileExists(const std::string& filename); // file exists function declaration
    void clientNameRead(); // client name read function declaration
    void clientIDRead(); // client id read function declaration
    void clientPrivateKeyRead(); // client private key read function declaration
    void sendRequest(const std::string& request); // send request function declaration
    void sendRequest(const std::vector<uint8_t>& request); // send request function declaration

    if (fileExists("me.info")) {
        clientNameRead();
        clientIDRead(); // Call the clientIDRead function
        try {
            clientPrivateKeyRead();
        }
        catch (const std::exception& e) {
            std::cerr << "Error reading the private key, please check the me.info file: " << e.what() << "\n";
            return;
        }
        clientReconnect();
    }
    

    else {
        //If the "me.info" file does not exist, trying to register
        clientRegister();
    }
}

void Client::sendRequest(const std::string& request) {
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket socket(io_service);
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::connect(socket, resolver.resolve({"localhost", "8000"}));
    try {
        boost::asio::write(socket_, boost::asio::buffer(request));
    }
    catch (std::exception) {
        std::cerr << "Error sending request" << std::endl;
    }
    
}

// function that sends a request to the server. 
void Client::sendRequest(const std::vector<uint8_t>& request) {
    // Create a buffer that contains the size of the request followed by the request data
    std::vector<uint8_t> buffer;
    // First, append the size of the request as a 4-byte integer (assuming uint32_t)
    uint32_t request_size = request.size();
    buffer.insert(buffer.end(), reinterpret_cast<uint8_t*>(&request_size), reinterpret_cast<uint8_t*>(&request_size) + sizeof(uint32_t));
    // Then, append the request data
    buffer.insert(buffer.end(), request.begin(), request.end());

    boost::asio::write(socket, boost::asio::buffer(request));
}

//function that prints the server response to the console.
void Client::printServerResponse(const Response& response) {
    std::cout << "Server Version: " << static_cast<int>(response.server_version) << std::endl;
    std::cout << "Response Code: " << response.response_code << std::endl;
    std::cout << "Payload Size: " << response.payload_size << std::endl;
    std::cout << "Payload Data: ";
    for (const auto& byte : response.payload) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte) << " ";
    }
    std::cout << std::endl;
}

// function to disconnect from the server
void Client::disconnectFromServer() {
    socket_.close();
}

// Reads data from the socket into the buffer_ and returns it as a string.
static std::string read()
{
    boost::system::error_code error;
    std::array<char, 1024> buffer_; // Declare the buffer_
    std::size_t len = boost::asio::read(socket, boost::asio::buffer(buffer_, buffer_.size()), error);
    if (error == boost::asio::error::eof)
        return "";
    else if (error)
        throw boost::system::system_error(error);
    return std::string(buffer_.begin(), buffer_.begin() + len);
}

// Reads n bytes of data from the socket into the buffer_ and returns it as a string.
static std::string read(std::size_t n)
{
    boost::system::error_code error;
    std::array<char, 1024> buffer_; // Declare the buffer_
    boost::asio::read(socket, boost::asio::buffer(buffer_, n), error);
    if (error == boost::asio::error::eof)
        return "";
    else if (error)
        throw boost::system::system_error(error);
    return std::string(buffer_.begin(), buffer_.begin() + n);
}

std::array<uint8_t, 2> decimalToTwoByteArray(uint16_t value) {
    std::array<uint8_t, 2> result;
    result[0] = value >> 8;
    result[1] = value & 0xFF;
    return result;
}

// Define the function decimalToFourByteArray
    std::array<uint8_t, 4> decimalToFourByteArray(size_t decimalValue) { // error here
        std::array<uint8_t, 4> byteArray;
        byteArray[0] = (decimalValue >> 24) & 0xFF;
        byteArray[1] = (decimalValue >> 16) & 0xFF;
        byteArray[2] = (decimalValue >> 8) & 0xFF;
        byteArray[3] = decimalValue & 0xFF;
        return byteArray;
    }

// function that receives a response from the server and returns it as a Response object.
Response Client::receiveResponse() {
    std::array<char, 1024> buffer_; // Declare the buffer_
    boost::system::error_code error; // Declare the error variable
    std::size_t bytesRead = socket_.read_some(boost::asio::buffer(buffer_), error); // Read data from the socket into the buffer_

    if (error) {
        throw boost::system::system_error(error); // Throw an exception if an error occurred
    }

    int response_code = 5;
    // Create and return the Response object
    Response response(response_code, std::vector<uint8_t>(buffer_.begin() +1, buffer_.begin() + bytesRead));
    return response;
}

void Client::clientRegister() {
    // Declare the variable reg_req
    Request reg_req;

    std::string client_name_ = "example_client"; // Declare and initialize the client_name_ variable

    std::array<uint8_t, 4> payload_size_arr = decimalToFourByteArray(client_name_.size());
    reg_req.request_code = decimalToTwoByteArray(1100);
    reg_req.payload_size = static_cast<uint32_t>(client_name_.size());
    uint32_t total_size = sizeof(reg_req.cid) + sizeof(reg_req.version) + sizeof(reg_req.request_code) + sizeof(reg_req.payload_size) + reg_req.payload_size;

    // Create a 255-byte ASCII string buffer for the client_name_
    std::array<char, 255> client_name_buffer_;
    std::copy(client_name_.begin(), client_name_.end(), client_name_buffer_.begin());

    // Construct the payload
    reg_req.payload.resize(client_name_.size());
    std::copy(client_name_.begin(), client_name_.end(), reg_req.payload.begin());

    // Construct the request
    std::vector<uint8_t> request(total_size);
    std::memcpy(request.data(), reg_req.cid.data(), sizeof(reg_req.cid));
    std::memcpy(request.data() + sizeof(reg_req.cid), &reg_req.version, sizeof(reg_req.version));
    std::memcpy(request.data() + sizeof(reg_req.cid) + sizeof(reg_req.version), reg_req.request_code.data(), sizeof(reg_req.request_code));
    std::memcpy(request.data() + sizeof(reg_req.cid) + sizeof(reg_req.version) + sizeof(reg_req.request_code), payload_size_arr.data(), sizeof(payload_size_arr));
    std::memcpy(request.data() + sizeof(reg_req.cid) + sizeof(reg_req.version) + sizeof(reg_req.request_code) + sizeof(payload_size_arr), reg_req.payload.data(), reg_req.payload.size());
}

void Client::clientReconnect() {
    // Prepare the header
    Request recon_req;
}

// function that send the client's public key to the server.
void Client::publicKeySend(const CryptoPP::RSA::PublicKey& public_key) {
    // Serialize the public key
    CryptoPP::ByteQueue queue;
    //std::array<uint8_t, 16> client_id_;
    Request public_key_req; // Declare the public_key_req variable

    public_key_req.request_code = decimalToTwoByteArray(1101);
    
    // Declare and initialize the missing variable public_key_size
    size_t public_key_size = 10;

    // Declare and initialize the missing variable client_name_buffer_
    std::array<char, 255> client_name_buffer_;
    
    public_key_req.payload_size = static_cast<uint32_t>(public_key_size + client_name_buffer_.size());
    uint32_t total_size = sizeof(public_key_req.cid) + sizeof(public_key_req.version) + sizeof(public_key_req.request_code) + sizeof(public_key_req.payload_size) + public_key_req.payload_size;

    // Create a 255-byte ASCII string buffer for the client_name_
    std::string client_name_; // Declare the missing variable client_name_;
    
    std::copy(client_name_.begin(), client_name_.end(), client_name_buffer_.begin());

    // Read the serialized public key into a vector
    std::vector<uint8_t> vec_key(public_key_size);
    
    std::vector<uint8_t> request(total_size); // Declare the request variable
    std::memcpy(request.data() + sizeof(public_key_req.cid) + sizeof(public_key_req.version), public_key_req.request_code.data(), sizeof(public_key_req.request_code));
    // Receive the response
    try {
        Response response = receiveResponse();
        
        printServerResponse(response);
        if (response.response_code == 2102) {
            constexpr size_t AES_KEY_SIZE = 16; // Define the missing identifier "AES_KEY_SIZE" with the appropriate value

            std::array<uint8_t, AES_KEY_SIZE> aes_key_; // Declare the aes_key_ variable

            // Declare and initialize the missing variable client_id_length
            size_t client_id_length = 5; /* add the appropriate value here */

            std::memcpy(aes_key_.data(), response.payload.data() + client_id_length, sizeof(aes_key_));
            if (aESKeyDecrypt()) {
                fileSend();
            }
            else {
                std::cerr << "Error: Decrypt the AES key by the private key failed";
                return;
            }

        }
        else {
            std::cerr << "Error: Failed to send the public key to the server" << std::endl;
        }
    }
    catch (const boost::system::system_error& e) {
        std::cerr << "Error: Failed to receive response from the server: " << e.what() << "\n";
    }
}

// Generates a 1024-bit RSA key pair (private and public keys) for the client,
void Client::generateKeysRSA() {
    // Set up a random number generator
    CryptoPP::AutoSeededRandomPool rng;

    // Generate a 1024-bit RSA key pair
    CryptoPP::InvertibleRSAFunction parameters;
    parameters.GenerateRandomWithKeySize(rng, 1024);

    // Create the private and public keys from the parameters
    CryptoPP::RSA::PrivateKey private_key_(parameters);
    CryptoPP::RSA::PublicKey public_key_(parameters);

    // Save the private key to a file
    privateKeySave(private_key_);
    publicKeySend(public_key_);
}

// function that returns a boolean indicating if the decryption was successful or not.
bool Client::aESKeyDecrypt() {
    try {

        // First, encrypt the AES key using the RSA private key
        //std::string encrypted_aes_key;
        // Declare the aes_key_ variable
        //uint8_t aes_key_[CryptoPP::AES::DEFAULT_KEYLENGTH];

        std::cerr << "The AES key was unlock successfully " << std::endl;
        return true;
    }
    catch (CryptoPP::Exception& e) {
        std::cerr << "Error unlocking AES key: " << e.what() << std::endl;
        return false;
    }
}

// Function that encrypts the file data using the AES key and returns a vector of encrypted data.
std::vector<uint8_t> Client::encryptFile() {
    // Check if the file exists
    // Declare and initialize the file_path_ variable with the appropriate file path
    std::string file_path_ = "/path/to/file";

    std::ifstream file(file_path_, std::ios::binary);
    if (!file) {
        throw std::runtime_error("File not found: " + file_path_);
    }
    // Read the file contents
    std::vector<uint8_t> file_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Generate a random IV
    CryptoPP::byte iv[CryptoPP::AES::BLOCKSIZE];
    CryptoPP::AutoSeededRandomPool rng;
    rng.GenerateBlock(iv, sizeof(iv));

    // Encrypt the file data using the AES key
    std::vector<uint8_t> encrypted_data; // Declare the encrypted_data variable
    try {
        CryptoPP::CFB_Mode<CryptoPP::AES>::Encryption encryption;
        CryptoPP::SecByteBlock decrypted_aes_key_(CryptoPP::AES::DEFAULT_KEYLENGTH);

        encryption.SetKeyWithIV(decrypted_aes_key_, CryptoPP::AES::DEFAULT_KEYLENGTH, iv);

        CryptoPP::StreamTransformationFilter stf(encryption, new CryptoPP::VectorSink(encrypted_data));
        stf.Put(file_data.data(), file_data.size()); // Add a semicolon at the end of the line
        stf.MessageEnd();
    }
    catch (const CryptoPP::Exception& e) {
        throw std::runtime_error("Error encrypting file: " + std::string(e.what()));
    }

    // append the IV to the beginning of the encrypted data
        encrypted_data.insert(encrypted_data.begin(), iv, iv + sizeof(iv));
    return encrypted_data;
    
}

//function to verify the CRC of the encrypted file data and compares it to the received CRC.
bool verify_crc(const std::vector<uint8_t>& encrypted_file_data, uint32_t received_crc) {
    boost::crc_32_type crc_calculator;
    crc_calculator.process_bytes(encrypted_file_data.data(), encrypted_file_data.size());
    uint32_t calculated_crc = crc_calculator.checksum();

    return calculated_crc == received_crc;
}

// function to sends a CRC invalid request to the server, indicating that the CRC does not match.
void Client::cRCInvalidRetry()
{
    // Declare and initialize the client_id_ variable
    int client_id_ = 12345; // Replace 12345 with the actual value
    Request invalid_crc_req;
    std::string file_path_ = "/path/to/file"; // Declare and initialize the file_path_ variable
    std::array<uint8_t, 4> client_id_bytes = {
        static_cast<uint8_t>((client_id_ >> 24) & 0xFF),
        static_cast<uint8_t>((client_id_ >> 16) & 0xFF),
        static_cast<uint8_t>((client_id_ >> 8) & 0xFF),
        static_cast<uint8_t>(client_id_ & 0xFF)
    };
    std::array<uint8_t, 4> payload_size_arr = decimalToFourByteArray(file_path_.size() + 1); // Declare the payload_size_arr variable
    //std::array<uint8_t, 4> vec = decimalToFourByteArray(file_path_.size() + 1); // Change the type of vec to std::array<uint8_t, 4>
    //std::array<uint8_t, 4> payload_size_arr = {vec[0], vec[1], vec[2], vec[3]};
    //payload_size_arr = vec; // Assign the value to payload_size_arr
    //invalid_crc_req.cid = client_id_bytes;
    
    std::vector<uint8_t> payload(file_path_.begin(), file_path_.end()); // Declare and initialize the payload vector

    uint32_t total_size = sizeof(invalid_crc_req.cid) + sizeof(invalid_crc_req.version) + sizeof(invalid_crc_req.request_code) + sizeof(payload_size_arr) + payload.size();
    std::vector<uint8_t> request(total_size);

    std::memcpy(request.data(), invalid_crc_req.cid.data(), sizeof(invalid_crc_req.cid));
    std::memcpy(request.data() + sizeof(invalid_crc_req.cid), &invalid_crc_req.version, sizeof(invalid_crc_req.version));
    std::memcpy(request.data() + sizeof(invalid_crc_req.cid) + sizeof(invalid_crc_req.version), invalid_crc_req.request_code.data(), sizeof(invalid_crc_req.request_code));
    std::memcpy(request.data() + sizeof(invalid_crc_req.cid) + sizeof(invalid_crc_req.version) + sizeof(invalid_crc_req.request_code), payload_size_arr.data(), sizeof(payload_size_arr));
    std::memcpy(request.data() + sizeof(invalid_crc_req.cid) + sizeof(invalid_crc_req.version) + sizeof(invalid_crc_req.request_code) + sizeof(payload_size_arr), payload.data(), payload.size());

    sendRequest(request);
    fileSend();
}

// CRC does not match and there are no additional attempts to resend the file.
void Client::cRCInvalidDone() {
    Request invalid_crc_req;
    #include <array> // Include the necessary header file

    int client_id_ = 12345; // Declare and initialize the client_id_ variable
    std::string file_path_ = "/path/to/file"; // Declare and initialize the file_path_ variable
    //invalid_crc_req.cid = client_id_;
    invalid_crc_req.request_code = decimalToTwoByteArray(1106);
    invalid_crc_req.payload_size = static_cast<uint32_t>(file_path_.size() + 1); // Include null terminator
    uint32_t total_size = sizeof(invalid_crc_req.cid) + sizeof(invalid_crc_req.version) + sizeof(invalid_crc_req.request_code) + sizeof(invalid_crc_req.payload_size) + invalid_crc_req.payload_size;

    std::cerr << "The size of the CRC invalid ,done  request is: " << total_size << "\n";
    std::array<uint8_t, 4> payload_size_arr; // Add the missing type specifier
    payload_size_arr = decimalToFourByteArray(file_path_.size() + 1); // Assign the value to payload_size_arr

    // Fill in the payload
    std::vector<uint8_t> payload(file_path_.begin(), file_path_.end());
    payload.push_back('\0');
    std::vector<uint8_t> invalid_crc_req_payload = payload; // Assign the payload to invalid_crc_req_payload
    invalid_crc_req.payload = invalid_crc_req_payload; // Assign the payload to invalid_crc_req

    // Construct the request
    std::vector<uint8_t> request(total_size);
    std::memcpy(request.data(), invalid_crc_req.cid.data(), sizeof(invalid_crc_req.cid));
    std::memcpy(request.data() + sizeof(invalid_crc_req.cid), &invalid_crc_req.version, sizeof(invalid_crc_req.version));
    std::memcpy(request.data() + sizeof(invalid_crc_req.cid) + sizeof(invalid_crc_req.version), invalid_crc_req.request_code.data(), sizeof(invalid_crc_req.request_code));
    std::memcpy(request.data() + sizeof(invalid_crc_req.cid) + sizeof(invalid_crc_req.version) + sizeof(invalid_crc_req.request_code), payload_size_arr.data(), sizeof(payload_size_arr));
    std::memcpy(request.data() + sizeof(invalid_crc_req.cid) + sizeof(invalid_crc_req.version) + sizeof(invalid_crc_req.request_code) + sizeof(payload_size_arr), payload.data(), payload.size());
    sendRequest(request);

    try {
        Response response = receiveResponse();
        printServerResponse(response);
        if (response.response_code == 2104) {

            std::cerr << "The CRC was incorrect for the fourth time, the file was not saved on the server because it failed to verify the CRC. Done";
            return;

        }
        else {
            std::cerr << "The server sent an unexpected response";
            return;
        }
    }
    catch (const boost::system::system_error& e) {
        std::cerr << "Error: Failed to receive response from the server: " << e.what() << "\n";
        return;

    }
}

// function to sends the encrypted file to the server and handles the server's response.
void Client::fileSend() {
    // Read the file contents
    std::string file_path_ = "/path/to/file"; // Declare and initialize the file_path_ variable
    std::ifstream file(file_path_, std::ios::binary);
    // Declare and initialize the client_id_ variable
    uint32_t client_id_ = 1234; // Replace 1234 with the actual client ID

    // Declare the req_send_file variable
    Request req_send_file;

    // Assign client_id_ to req_send_file.cid
    //req_send_file.cid = client_id_;

    std::vector<uint8_t> file_name_bytes; // Declare and initialize the file_name_bytes variable
    // Declare and initialize the encrypted_file_data variable
    std::vector<uint8_t> encrypted_file_data; // Replace with the actual encrypted file data

    file_name_bytes.push_back('\0'); // Add null terminator to file_name_bytes

    // Declare and initialize the file_size variable
    uint32_t file_size = 0; // Replace 0 with the actual file size

    // Declare and initialize the file_size_bytes variable
    std::array<uint8_t, 4> file_size_bytes = decimalToFourByteArray(file_size);

    // Construct the payload
    req_send_file.payload.resize(4 + 255 + encrypted_file_data.size());
    std::memcpy(req_send_file.payload.data(), file_size_bytes.data(), file_size_bytes.size());
    std::memcpy(req_send_file.payload.data() + 4, file_name_bytes.data(), 255);
    std::memcpy(req_send_file.payload.data() + 4 + 255, encrypted_file_data.data(), encrypted_file_data.size());

    std::array<uint8_t, 4> payload_size_arr = decimalToFourByteArray(req_send_file.payload.size());
    // Declare the req_send_file variable
    
    uint32_t total_size = sizeof(req_send_file.cid) + sizeof(req_send_file.version) + sizeof(req_send_file.request_code) + sizeof(req_send_file.payload_size) + req_send_file.payload_size;
    std::cerr << "The size of the request to send a file is: " << total_size << "\n";

    // Construct the request
    std::vector<uint8_t> request(total_size);
    std::memcpy(request.data(), req_send_file.cid.data(), sizeof(req_send_file.cid));
    std::memcpy(request.data() + sizeof(req_send_file.cid), &req_send_file.version, sizeof(req_send_file.version));
    std::memcpy(request.data() + sizeof(req_send_file.cid) + sizeof(req_send_file.version), req_send_file.request_code.data(), sizeof(req_send_file.request_code));
    std::memcpy(request.data() + sizeof(req_send_file.cid) + sizeof(req_send_file.version) + sizeof(req_send_file.request_code), payload_size_arr.data(), sizeof(payload_size_arr));
    std::memcpy(request.data() + sizeof(req_send_file.cid) + sizeof(req_send_file.version) + sizeof(req_send_file.request_code) + sizeof(payload_size_arr), req_send_file.payload.data(), req_send_file.payload.size());

    // Send the request to the server
    sendRequest(request);

    // Wait for the server's response
    Response response = receiveResponse();
    printServerResponse(response);
    // Check if the response is valid
    if (response.response_code == 2103  ) {
        uint32_t received_crc = *reinterpret_cast<const uint32_t*>(response.payload.data() + 16 + 4 + 255);
        if (verify_crc(encrypted_file_data, received_crc)) {
            std::cout << "File sent successfully and CRC matches." << std::endl;
            cRCvalidate();
        }
        else if(error_count_< 3){
            std::cerr << "File sent successfully, but CRC does not match." << std::endl;
            error_count_++;
            std::cerr << "Sending a request to the server that the CRC is incorrect" << std::endl;
            cRCInvalidRetry();
        }
        else {
            std::cerr << "File sent successfully, but CRC does not match for the fourth time." << std::endl;
            error_count_++;
            std::cerr << "Finishing trying to send the file" << std::endl;
            cRCInvalidDone();
        }
    }
    else {
        std::cerr << "Error sending file to the server." << std::endl;
        disconnectFromServer();
    }
}

// function that verify if a file exists at the specified path.
void Client::cRCvalidate() {
    // Declare and initialize the client_id_ variable
    std::string file_path_ = "/path/to/file"; // Replace "/path/to/file" with the actual file path

    uint32_t client_id_ = 1234; // Replace 1234 with the actual client ID

    Request valid_crc_req;
    //valid_crc_req.cid = client_id_;
    valid_crc_req.request_code = decimalToTwoByteArray(1104);
    valid_crc_req.payload_size = static_cast<uint32_t>(file_path_.size() + 1); // Include null terminator
    uint32_t total_size = sizeof(valid_crc_req.cid) + sizeof(valid_crc_req.version) + sizeof(valid_crc_req.request_code) + sizeof(valid_crc_req.payload_size) + valid_crc_req.payload_size;

    std::cerr << "The size of the CRC valid request is: " << total_size << "\n";
    std::array<uint8_t, 4> payload_size_arr = decimalToFourByteArray(file_path_.size() + 1);

    // Fill in the payload
    std::vector<uint8_t> payload(file_path_.begin(), file_path_.end());
    payload.push_back('\0');
    valid_crc_req.payload = payload;
    // Declare the request variable
    std::vector<uint8_t> request(total_size);
    // Construct the request
    std::memcpy(request.data(), valid_crc_req.cid.data(), sizeof(valid_crc_req.cid));
    std::memcpy(request.data() + sizeof(valid_crc_req.cid), &valid_crc_req.version, sizeof(valid_crc_req.version));
    std::memcpy(request.data() + sizeof(valid_crc_req.cid) + sizeof(valid_crc_req.version), valid_crc_req.request_code.data(), sizeof(valid_crc_req.request_code));
    std::memcpy(request.data() + sizeof(valid_crc_req.cid) + sizeof(valid_crc_req.version) + sizeof(valid_crc_req.request_code), payload_size_arr.data(), sizeof(payload_size_arr));
    std::memcpy(request.data() + sizeof(valid_crc_req.cid) + sizeof(valid_crc_req.version) + sizeof(valid_crc_req.request_code) + sizeof(payload_size_arr), payload.data(), payload.size());

    sendRequest(request);

    try {
        Response response = receiveResponse();
        printServerResponse(response);
        if (response.response_code == 2104) {

            std::cerr << "The server confirmed receipt of the file, the file transfer was completed successfully. Thank you for saving the file on our servers";
            return;
            
        }
        else {
            std::cerr << "The server sent an unexpected response";
            return;
        }
    }
    catch (const boost::system::system_error& e) {
        std::cerr << "Error: Failed to receive response from the server: " << e.what() << "\n";
        return;
        
    }

}


// function to register the client with the server.
void Client::connectToServer() {
    // Declare and initialize the server_endpoint_ variable
    boost::asio::ip::tcp::endpoint server_endpoint_(boost::asio::ip::address::from_string("127.0.0.1"), 8080); // Replace "127.0.0.1" and 8080 with the actual server IP address and port number

    try {
        socket_.connect(server_endpoint_);
    }
    catch (boost::system::system_error& e) {
        // Handle the error (e.g., print an error message or throw an exception)
        std::cerr << "Error: Failed to connect to the server: " << e.what() << "\n";
        throw e;
    }
}

// Saves the client name to a file named "me.info"
void Client:: clientSaveName(const std::string& client_name) {
    std::ofstream file("me.info", std::ios::out | std::ios::trunc);
    if (file.is_open()) {
        file << client_name << std::endl;
        file.close();
    }
    else {
        std::cerr << "Error opening file 'me.info' for writing" << std::endl;
    }
}

// Saves the client ID (as a series of hexadecimal bytes) to the "me.info" file
void Client::clientSaveID(const std::vector<uint8_t>& data) {
    std::ofstream file("me.info", std::ios::out | std::ios::app);
    if (file.is_open()) {
        for (const auto& byte : data) {
            file << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        file << std::endl;
        file.close();
    }
    else {
        std::cerr << "Error opening file 'me.info' for writing" << std::endl;
    }
}

//  Saves the private key (in Base64 format) to the "me.info" file
void Client::privateKeySave(const CryptoPP::RSA::PrivateKey& private_key_) {
    std::ofstream file("me.info", std::ios::out | std::ios::app);
    if (file.is_open()) {
        CryptoPP::ByteQueue privateKeyQueue;
        private_key_.Save(privateKeyQueue);
        CryptoPP::Base64Encoder base64Encoder(new CryptoPP::FileSink(file), false);
        privateKeyQueue.CopyTo(base64Encoder);
        base64Encoder.MessageEnd();

        file << std::endl;
        file.close();
    }
    else {
        std::cerr << "Error opening file 'me.info' for writing" << std::endl;
    }
}

// Reads the client name from the "me.info" file
void Client::clientNameRead() {
        std::ifstream file("me.info");
        std::string client_name_; // Declare the variable client_name_
        std::getline(file, client_name_);
        file.close();
    }

    // Reads the client ID (as a series of hexadecimal bytes) from the "me.info" file
    void Client::clientIDRead() {
        std::ifstream file("me.info");
        std::string line;
        std::getline(file, line);
        std::getline(file, line);
        std::vector<uint8_t> client_id_(line.size() / 2);
        for (size_t i = 0; i < client_id_.size(); i++) {
            std::string hex_byte = line.substr(i * 2, 2);
            client_id_[i] = std::stoi(hex_byte, nullptr, 16);
        }
        file.close();
    }

    // Reads the private key (in Base64 format) from the "me.info" file
    void Client::clientPrivateKeyRead() {
        std::ifstream file("me.info");
        std::string line;
        std::getline(file, line);
        std::getline(file, line);
        std::getline(file, line);
        CryptoPP::ByteQueue queue;
        CryptoPP::StringSource string_source(line, true, new CryptoPP::Base64Decoder);
        string_source.TransferTo(queue);
        queue.MessageEnd();
        private_key_.Load(queue);
        file.close();
    }

    // Checks if a file exists at the given file path
    bool Client:: fileExists(const std::string& file_path) {
        std::ifstream file(file_path);
        bool exists = file.good();
        file.close();
        return exists;
    }

    int main() {
        
        std::string ip, client_name, file_path;
    int port;

    // Read the transfer.info file and get the required information
    std::ifstream info_file("transfer.info");
    if (info_file.is_open()) {
        std::getline(info_file, ip);
        size_t colonPos = ip.find(':');
        port = std::stoi(ip.substr(colonPos + 1));
        ip = ip.substr(0, colonPos);

        // Read the client name from the second line
        std::getline(info_file, client_name);

        // Read the file path from the third line
        std::getline(info_file, file_path);
        info_file.close();
    }
    else {
        std::cerr << "Error: Failed to open transfer.info file\n";
        return 1;
    }

    // Create an io_context object
    boost::asio::io_context io_context;

    //Client client(io_context, ip, port, client_name, file_path);
    client.run();

    // Clean up
    client.disconnectFromServer();

    return 0;
}
