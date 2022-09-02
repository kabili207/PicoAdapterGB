//GPIO23 - LED RGB WS2812 (NeoPixelConnect)
//GPIO25 - LED_BUILTIN

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

//Custom Headers
#include "esp_at.h"

#define WiFiSSID "WiFi_Network"
#define WiFiPSK "P@$$w0rd"

#define LED_PIN         25
#define LED_SET(A)      (gpio_put(LED_PIN, (A)))
#define LED_ON          LED_SET(true)
#define LED_OFF         LED_SET(false)
#define LED_TOGGLE      (gpio_put(LED_PIN, !gpio_get(LED_PIN)))

#define SEC(A)                  ((A) * 1000 * 1000)

#define UART_ID uart1
#define BAUD_RATE 115200
#define UART_TX_PIN 4
#define UART_RX_PIN 5

#define MAGB_HOST "192.168.0.126"
#define MAGB_PORT 80


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void main() {
    sleep_ms(2000); 
    // Enable UART so we can print
    stdio_init_all();

    // For toggle_led
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    LED_ON;

    EspAT_Init(UART_ID, BAUD_RATE, UART_TX_PIN, UART_RX_PIN);
    sleep_ms(2000); 
//////////////////////
// CONFIGURE THE ESP
//////////////////////
    //test connection - Check 2 times in case the ESP return some echo 
    FlushATBuff(); // Reset Buffer array
    SendESPcmd(UART_ID,"AT");
    char * resp = ReadESPcmd(SEC(2));
    if(strcmp(resp, "OK") == 0){
        printf("ESP-01 Connectivity: OK\n");
    }else{        
        printf("ESP-01 Connectivity: ERROR || Retrying...\n");
        resp = ReadESPcmd(SEC(2));
        if(strcmp(resp, "OK") == 0){
            printf("ESP-01 Connectivity: OK\n");
        }else{        
            printf("ESP-01 Connectivity: ERROR\n");
        }
    }

    // Disable echo - Check 2 times in case the ESP return some echo 
    SendESPcmd(UART_ID,"ATE0");
    resp = ReadESPcmd(SEC(2));
    if(strcmp(resp, "OK") == 0){
        printf("ESP-01 Disable Echo: OK\n");
    }else{        
        printf("ESP-01 Disable Echo: ERROR || Retrying...\n");
        resp = ReadESPcmd(SEC(2));
        if(strcmp(resp, "OK") == 0){
            printf("ESP-01 Disable Echo: OK\n");
        }else{        
            printf("ESP-01 Disable Echo: ERROR\n");
        }
    }

    // Set to Passive Mode to receive TCP info
    // AT+CIPRECVDATA=<size> | read the X amount of data from esp buffer
    // AT+CIPRECVLEN? | return the remaining  buffer size like this +CIPRECVLEN:636,0,0,0,0)
    // Also can read the actual size reading the +IPD value from "AT+CIPSEND" output: \r\n\r\nRecv 60 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,636\r\nCLOSED\r\n
    SendESPcmd(UART_ID, "AT+CIPRECVMODE=1");
    resp = ReadESPcmd(SEC(2));
    if(strcmp(resp, "OK") == 0){
        printf("ESP-01 Passive Mode: OK\n");
    }else{        
        printf("ESP-01 Passive Mode: ERROR\n");
    }

    if(ConnectESPWiFi(UART_ID, WiFiSSID, WiFiPSK,SEC(10))){
        bool reqStatus = SendESPGetReq(UART_ID, MAGB_HOST, MAGB_PORT, "/01/CGB-B9AJ/index.php");
        ReadESPGetReq(UART_ID,700); //The value will be stored into buffGETReq array
    }else{
        printf("ESP-01 Connecting Wifi: ERROR\n");
    }
//////////////////
// END CONFIGURE
//////////////////
   
    //Need a request data now. (from esp buffer)    
    // AT+CIPRECVLEN? | return the remaining  buffer size like this +CIPRECVLEN:636,0,0,0,0 ... but the Variable ipdVal hold this value and need to subtract each reading
    // AT+CIPRECVDATA=<size> | read the X amount of data from esp buffer

    //ishttpRequest=true;
    //SendESPcmd(UART_ID,"AT+CIPRECVDATA=300"); //Must igonre the OK at the end, and the +CIPRECVDATA,<size> at the beginning
    //RunTimeout(SEC(2));    
    //ishttpRequest=false;
    //for(int y = 0; y < strlen(buffATrx); y++){
    //    //if (buffATrx[y] == buffDelimiter){
    //    //    printf("\r");
    //    //    printf("\n");
    //    //}else{
    //    //    printf("%c",buffATrx[y]);
    //    //}
    //    printf("%c",buffATrx[y]);
    //}

    printf("\nDone\n");
    LED_OFF;

    while (true) {
        sleep_ms(1000);
    }
}