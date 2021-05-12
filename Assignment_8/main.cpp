#include "mbed.h"
#include <string.h>



struct Counter 
{
    int minute;
    int second;
    Mutex mutex;

};

void minute(Counter* counter)
{
    
    while (true) {
        ThisThread::sleep_for(60s);

        counter->mutex.lock();
        counter->minute += 1;
        counter->mutex.unlock();

    }
}

void second(Counter* counter)
{
    while (true) {
        ThisThread::sleep_for(1s);

        if (counter->second >= 60)
        {
        counter->mutex.lock();
        counter->second = 0;
        counter->mutex.unlock();
        }
        
        else
        {
        counter->mutex.lock();
        counter->second +=1;
        counter->mutex.unlock();
        }

    }
}

void print_clock(Counter* counter)
{
    while (true) {
        ThisThread::sleep_for(500ms);

        counter->mutex.lock();
        printf("Current clock value: %i minute(s) and %i second(s).\n", counter->minute, counter->second);
        counter->mutex.unlock();


    }
}



int main()
{
    Counter counter;
    

    Thread thread1;
    Thread thread2;
    Thread thread3;

    thread1.start(callback(minute, &counter));
    thread2.start(callback(second, &counter));
    thread3.start(callback(print_clock, &counter));

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
