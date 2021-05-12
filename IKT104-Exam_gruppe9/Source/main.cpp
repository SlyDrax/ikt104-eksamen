#include "mbed.h"

#define JSON_NOEXCEPTION
#include "json.hpp"
#include <stdio.h>
#include <time.h>
#include "wifi.h"
#include "cert.h"

#include "DFRobot_RGBLCD.h"


using json = nlohmann::json;

DFRobot_RGBLCD lcd(16,2,D14,D15); 

Timer t;
int timer;

int main()
{
    lcd.init();
    // Get pointer to default network interface
    NetworkInterface *network = NetworkInterface::get_default_instance();

    if (!network) {
        printf("Failed to get default network interface\n");
        while (1);
    }

    // Connect to network
    printf("Connecting to the network...\n");
    nsapi_size_or_error_t result = network->connect();

    // Check if the connection is successful
    if (result != 0) {
        printf("Failed to connect to network: %d\n", result);
        while (1);
    }

    printf("Connection to network successful!\n");

    // Create and allocate resources for socket
    TLSSocket *socket = new TLSSocket();

    result = socket->set_root_ca_cert(cert);
    if (result != 0) {
        printf("Error: socket->set_root_ca_cert() returned &d\n", result);
        
    }

    socket->open(network);

    // Create destination address
    SocketAddress address;

    result = network->gethostbyname("api.ipgeolocation.io", &address);

    // Check result
    if (result != 0) {
        printf("Failed to get IP address of host: %d\n", result);
        while (1);
    }

    printf("Got address of host\n");

      // Set server port number
    address.set_port(443);

    // Connect to server at the given address
    result = socket->connect(address);

    // Check result
    if (result != 0) {
        printf("Failed to connect to server: %d\n", result);
        while (1);
    }
    printf("Connection to server successful!\n");

// Create HTTP request
    const char request[] = "GET /timezone?apiKey=29c4499bedc946199569f5541ec7d8a4&tz HTTP/1.1\r\n"
                           "Host: api.ipgeolocation.io\r\n"
                           "Connection: close\r\n"
                           "\r\n";

    // Send request
    result = send_request(socket, request);

    // Check result
    if (result != 0) {
        printf("Failed to send request: %d\n", result);
        while (1);
    }
    
    // Read response
    static char buffer[4096];
    result = read_response(socket, buffer, sizeof(buffer));

    // Check result
    if (result != 0) {
        printf("Failed to read response: %d\n", result);
        while (1);
    }

    socket->close();

    char* json_begin = strchr(buffer, '{');
    char* json_end = strrchr(buffer, '}');

    if (json_begin == nullptr || json_end == nullptr) {
        printf("Failed to find JSON in response\n");
        while(1);
    }

    json_end[1] = 0;

    printf("JSON response:\n");
    printf("%s\n", json_begin);

    json document = json::parse(json_begin);


    std::string tid;
    time_t unix_timer;
    int timezone_offset;
    int dst_savings;
    struct tm * timeinfo = {0};
    document["date_time_unix"].get_to(unix_timer);
    document["timezone_offset"].get_to(timezone_offset);
    document["dst_savings"].get_to(dst_savings);

    
    timeinfo = localtime (&unix_timer);
    timeinfo->tm_hour += timezone_offset+dst_savings;
    mktime(timeinfo);

    char buffer2[80];
    strftime (buffer2,80,"\nTimedate is: %c\n", timeinfo);
    puts (buffer2);
    lcd.clear();
    lcd.printf(buffer2);
    

    while (true) {

        ThisThread::sleep_for(1s);
    }
}
