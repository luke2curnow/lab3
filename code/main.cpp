

#include "mbed.h"
#include "arm_book_lib.h"



#define UPDATE_INTERVAL 3s  
#define GAS_LIMIT 500   
#define TEMP_LIMIT 50   


DigitalIn enterButton(BUTTON1);
DigitalIn gasDetector(D2);
DigitalIn overTempDetector(D3);
DigitalIn aButton(D4);
DigitalIn bButton(D5);
DigitalIn cButton(D6);
DigitalIn dButton(D7);

DigitalOut alarmLed(LED1);
DigitalOut incorrectCodeLed(LED3);
DigitalOut systemBlockedLed(LED2);

UnbufferedSerial uartUsb(USBTX, USBRX, 115200);
Ticker periodicUpdate; 


bool alarmState = OFF;
int numberOfIncorrectCodes = 0;
int checkgas = 0;
int checktemp = 0;


void inputsInit();
void outputsInit();
void periodicSendData();

void alarmActivationUpdate();
void alarmDeactivationUpdate();

void uartTask();
void availableCommands();
void reportDetectorStatus(char command);
void check_gas();
void check_temp();


int main()
{
    inputsInit();
    outputsInit();

        periodicUpdate.attach(&periodicSendData, UPDATE_INTERVAL);

    while (true) {
        alarmActivationUpdate();
        alarmDeactivationUpdate();
        uartTask();
    }
}



void check_gas() {
  
    if (checkgas > GAS_LIMIT) {
        uartUsb.write("Gas level dangerous\n", 21); 
    }
}

void check_temp() {
    if (checktemp > TEMP_LIMIT) {
        uartUsb.write("Temperature level dangerous\n", 28);
    }
}

void periodicSendData()
{
    bool gasDetected = gasDetector.read();
    bool tempDetected = overTempDetector.read();

    char buffer[100];
    snprintf(buffer, sizeof(buffer),
             "ALARM: %s | TEMP: %s | GAS: %s\r\n",
             (gasDetected || tempDetected) ? "ON" : "OFF",
             tempDetected ? "Activated" : "Not Active",
             gasDetected ? "Activated" : "Not Active");

    uartUsb.write(buffer, strlen(buffer));
    check_gas();  
    check_temp(); 
}

void inputsInit()
{
    gasDetector.mode(PullDown);
    overTempDetector.mode(PullDown);
    aButton.mode(PullDown);
    bButton.mode(PullDown);
    cButton.mode(PullDown);
    dButton.mode(PullDown);
}

void outputsInit()
{
    alarmLed = OFF;
    incorrectCodeLed = OFF;
    systemBlockedLed = OFF;
}

void alarmActivationUpdate()
{
    bool gasDetected = gasDetector.read();
    bool tempDetected = overTempDetector.read();
    
    if (gasDetected || tempDetected) {
        alarmState = ON;
    } else {
        alarmState = OFF; 
    }

    alarmLed = alarmState;
}

void alarmDeactivationUpdate()
{
    if (numberOfIncorrectCodes < 5) {
        if (aButton && bButton && cButton && dButton && !enterButton) {
            incorrectCodeLed = OFF;
        }
        if (enterButton && !incorrectCodeLed && alarmState) {
            if (aButton && bButton && !cButton && !dButton) {
                alarmState = OFF;
                numberOfIncorrectCodes = 0;
            } else {
                incorrectCodeLed = ON;
                numberOfIncorrectCodes++;
            }
        }
    } else {
        systemBlockedLed = ON;
    }
}

void uartTask()
{
    char receivedChar = '\0';

    if (uartUsb.readable()) {
        uartUsb.read(&receivedChar, 1);

        switch (receivedChar) {
            case '1':
                if (alarmState) {
                    uartUsb.write("The alarm is activated\r\n", 24);
                } else {
                    uartUsb.write("The alarm is not activated\r\n", 28);
                }
                break;
            case '2':
            case '3':
                reportDetectorStatus(receivedChar);
                break;
            default:
                availableCommands();
                break;
        }
    }
}

void reportDetectorStatus(char command)
{
    if (command == '2') {
        uartUsb.write("\r\n", 2); 
        if (overTempDetector.read()) {
            uartUsb.write("Temperature sensor is activated\r\n\n", 34);
        } else {
            uartUsb.write("Temperature sensor is not active\r\n", 37);
        }
    } else if (command == '3') {
        uartUsb.write("\r\n", 2);
        if (gasDetector.read()) {
            uartUsb.write("Gas sensor is activated\r\n", 26);
        } else {
            uartUsb.write("Gas sensor is not active\r\n", 28);
        }
    }
}

void availableCommands()
{
    uartUsb.write("Available commands:\r\n", 21);
    uartUsb.write("Press '1' to get the alarm state\r\n", 36);
    uartUsb.write("Press '2' to check the temperature sensor\r\n", 42);
    uartUsb.write("Press '3' to check the gas sensor\r\n", 36);
}
