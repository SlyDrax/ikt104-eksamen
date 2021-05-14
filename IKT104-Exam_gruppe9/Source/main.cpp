#include "mbed.h"
#include <chrono>

#include <iostream>

#define JSON_NOEXCEPTION
#include "json.hpp"
#include <stdio.h>
#include <time.h>
#include "wifi.h"
#include "cert.h"
#include "HTS221Sensor.h"

#include "DFRobot_RGBLCD.h"


using json = nlohmann::json;

DFRobot_RGBLCD lcd(16,2,D14,D15); 

int colorR = 255;
int colorG = 255;
int colorB = 255;

// doubles as +
InterruptIn b2(A0, PullUp);
InterruptIn b1(A1, PullUp);
// doubles as -
InterruptIn b3(A2, PullUp);

PwmOut buzzer(D11);

int screen_toggle = 1;
int mod_toggle = 0;

Timer t;
int timer;

int set_alarm[2] = {0};

bool is_armed = 0;

void forward_function() {
    if(mod_toggle == 1){
        // Hours
        set_alarm[0] += 1;
        if(set_alarm[0] >= 24)
            set_alarm[0] = 0;
    }
    else if(mod_toggle == 2) {
        // Minutes
        set_alarm[1] += 1;
        if(set_alarm[1] >= 60)
            set_alarm[1] = 0;            
    }
    else {
        if(screen_toggle >= 4)
            screen_toggle = 1;
        else  screen_toggle += 1;
    }
}

void mod_function(){
    if(screen_toggle == 2)
        mod_toggle += 1;
    else if(screen_toggle == 5){
        mod_toggle += 1;
        screen_toggle = 1;
        is_armed = 0;
        }
}

void back_function() {
    if(mod_toggle == 1){
        // Hours
        set_alarm[0] -= 1;
        if(set_alarm[0] < 0)
            set_alarm[0] = 23;
    }
    else if(mod_toggle == 2) {
        // Minutes
        set_alarm[1] -= 1;
        if(set_alarm[1] < 0)
            set_alarm[1] = 59; 
    }
    else if(screen_toggle == 5){
        // snooze
        set_alarm[1] += 5;
        mod_toggle = 1;
        is_armed = 1;
        if(set_alarm[1] >= 60){
            set_alarm[1] -= 60;
            set_alarm[0] += 1;
            if(set_alarm[0] >= 24){
                set_alarm[0] = 0;
            }
        }
    }
    else {
        if(screen_toggle <= 1)
            screen_toggle = 4;
        else  screen_toggle -= 1;
    }
}

void trigger_mod() {mod_function();}
void trigger_forward() {forward_function();}
void trigger_back() {back_function();}


void time_update(tm* timeinfo) {
    char buffer[16];
    
    strftime (buffer,16,"%a %B %d %G", timeinfo);

    int hr = timeinfo->tm_hour;
    int min = timeinfo->tm_min;
    int sec = timeinfo->tm_sec;

    printf("%02i:%02i:%02i\n",hr, min, sec);
    lcd.clear();
    lcd.printf("%02i:%02i:%02i", hr, min, sec);
    lcd.setCursor(0,1);
    lcd.printf("%s", buffer);
}

float rand_float(){
    return (float)rand()/(float)(RAND_MAX/2)+0.1;
}

void beep(){
    printf("BEEP!\n");
    buzzer.period((float)1/3500);
    buzzer.write(0.05f);
    lcd.setRGB(255, 0, 0);
    ThisThread::sleep_for(50ms);
    buzzer.write(0.0);
    ThisThread::sleep_for(50ms);
    buzzer.write(0.05f);
    lcd.setRGB(0, 255, 0);
    ThisThread::sleep_for(50ms);
    buzzer.write(0.0);
    ThisThread::sleep_for(50ms);
    buzzer.write(0.05f);
    lcd.setRGB(0, 0, 255);
    ThisThread::sleep_for(50ms);
    buzzer.write(0.0);
    ThisThread::sleep_for(50ms);
    buzzer.write(0.05f);
    lcd.setRGB(255, 0, 0);
    ThisThread::sleep_for(50ms);
    buzzer.write(0.0);
    ThisThread::sleep_for(50ms);
    buzzer.write(0.05f);
    lcd.setRGB(0, 255, 0);
    ThisThread::sleep_for(200ms);
    lcd.setRGB(0, 0, 255);
    buzzer.write(0.0);
    printf("%.06f\n", rand_float());
}

void alarm_check(tm* timeinfo){
    if(!is_armed)
        return;
    if((set_alarm[0] == timeinfo->tm_hour) && (set_alarm[1] == timeinfo->tm_min)){
        printf("toggeling screen 5...");
        screen_toggle = 5;
        printf("Setalarm == time");
        while(is_armed){
            time_t local_seconds = time(NULL);
            struct tm* local_timeinfo = localtime(&local_seconds);
            lcd.clear();
            lcd.printf("      ALARM     ");
            lcd.setCursor(0,1);
            lcd.printf("      %02i:%02i      ", local_timeinfo->tm_hour, local_timeinfo->tm_min);

            beep();
            if(mod_toggle){
                mod_toggle += 1;
                screen_toggle = 1;
                break;
            }
        }
    }
}

void bebug(){
    printf("screen toggle %i, mod: %i, is_armed: %i\n", screen_toggle, mod_toggle, is_armed);
}

void alarm_view() {
    
    lcd.clear();
    lcd.printf("Set alarm: %02i:%02i", set_alarm[0],set_alarm[1]);

    while(true){
        
        if (mod_toggle == 1) {
            while(true){
                int hr = set_alarm[0];
                int min = set_alarm[1];
                lcd.clear();
                lcd.printf("Set alarm:   :%02i", min);
                ThisThread::sleep_for(300ms);
                lcd.clear();
                lcd.printf("Set alarm: %02i:%02i", hr, min);
                ThisThread::sleep_for(300ms);

                if (mod_toggle !=1)
                    break;
                }
            
        }
        else if (mod_toggle == 2) {
            while(true){
                int hr = set_alarm[0];
                int min = set_alarm[1];
                lcd.clear();
                lcd.printf("Set alarm: %02i:  ", hr);
                ThisThread::sleep_for(300ms);
                lcd.clear();
                lcd.printf("Set alarm: %02i:%02i", hr, min);
                ThisThread::sleep_for(300ms);

                if (mod_toggle !=2)
                    is_armed = 1;
                    break;
                }
            
        }
        else { 
        
        printf("resetting mod_toggle, old value: %i\n", mod_toggle);
        mod_toggle = 0;

        
            break;}
    
    }
    

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

void print_temp(float var) {
    printf("Temp: %.2f ", var);
    lcd.printf("Temp: %.1fC", var);
}

void print_hum(float var) {
    printf("Hum: %.2f\n", var);
    lcd.setCursor(0,1);
    lcd.printf("Hum:  %.1f", var);
}

void hum_temp(){
    DevI2C i2c(PB_11, PB_10);
    HTS221Sensor sensor(&i2c);
    sensor.init(nullptr);
    sensor.enable();
    float temperature;
    float humidity;
    lcd.clear();
    if (sensor.get_temperature(&temperature) != 0)
        printf("An error occured reading temperature\n");
    else print_temp(temperature);

    if (sensor.get_humidity(&humidity) != 0)
        printf("An error occured reading humidity\n");
    else print_hum(humidity);
}

int main()
{
    set_time(setup());

    lcd.init();
    lcd.setRGB(colorR, colorG, colorB);
    b1.fall(&trigger_forward);
    b2.fall(&trigger_mod);
    b3.fall(&trigger_back);

    
    while (true) {
        time_t seconds = time(NULL);
        struct tm* timeinfo = localtime(&seconds);

        alarm_check(timeinfo);
        bebug();
        
        if(screen_toggle == 1)
            time_update(timeinfo);
        else if(screen_toggle == 2)
            alarm_view();
        else if(screen_toggle == 3)
            hum_temp();
        mod_toggle = 0;
        ThisThread::sleep_for(1s);
        lcd.setRGB(255, 255, 255);
    }
}
