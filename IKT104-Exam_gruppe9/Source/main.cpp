#include "mbed.h"
#include <chrono>

#include <iostream>


#define JSON_NOEXCEPTION
#include "json.hpp"
#include <stdio.h>
#include <time.h>
#include "wifi.h"
#include "cert.h"


#include "DFRobot_RGBLCD.h"


using json = nlohmann::json;

DFRobot_RGBLCD lcd(16,2,D14,D15); 

// doubles as +
InterruptIn b1(A0, PullUp);
InterruptIn b2(A1, PullUp);
// doubles as -
InterruptIn b3(A2, PullUp);

int screen_toggle = 1;
bool mod_toggle = 0;

Timer t;
int timer;

void trigger_forward() {
    if(mod_toggle) {
    }
    else {
        if(screen_toggle >= 4)
            screen_toggle = 1;
        else  screen_toggle += 1;
    }
}
void trigger_mod();

void trigger_back() {
    if(mod_toggle) {
        
    }
    else {
        if(screen_toggle <= 1)
            screen_toggle = 4;
        else  screen_toggle -= 1;
    }
}

void time_update() {
    char buffer2[80];
    char buffer3[16];
    time_t seconds = time(NULL);
    strftime (buffer2,80,"%T", localtime(&seconds));
    strftime (buffer3,16,"%a %B %d %G", localtime(&seconds));
    printf("%s\n",buffer2);
    lcd.clear();
    lcd.printf("%s", buffer2);
    lcd.setCursor(0,1);
    lcd.printf("%s", buffer3);
}

void alarm_view() {


}

json json_reader(char buffer[]) {
    char* json_begin = strchr(buffer, '{');
    char* json_end = strrchr(buffer, '}');

    if (json_begin == nullptr || json_end == nullptr) {
        printf("Failed to find JSON in response\n");
        while(1);
    }

    json_end[1] = 0;

    printf("JSON response:\n");
    printf("%s\n", json_begin);

    return json::parse(json_begin);
}

unsigned long int setup() {
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

    json document = json_reader(buffer);

    std::string tid;
    time_t unix_timer;
    int timezone_offset;
    int dst_savings;
    document["date_time_unix"].get_to(unix_timer);
    document["timezone_offset"].get_to(timezone_offset);
    document["dst_savings"].get_to(dst_savings);

    return unix_timer+timezone_offset*3600+dst_savings*3600;
}

int main()
{
    set_time(setup());

    lcd.init();

    b1.fall(&trigger_forward);
    // b2.fall(&trigger_mod);
    b3.fall(&trigger_back);

    while (true) {

        printf("%i\n", screen_toggle);

        if(screen_toggle == 1)
            time_update();

        ThisThread::sleep_for(1s);
    }
}
