//GPIO23 - LED RGB WS2812 (NeoPixelConnect)
//GPIO25 - LED_BUILTIN

#include <stdio.h>
#include <string.h>
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

//////////////////////
// CONFIGURE THE ESP
//////////////////////
    //test connection (return echo)    
    FlushCmdBuff();
    SendESPcmd(UART_ID,"AT");
    ReadESPcmd(SEC(2));
    FlushCmdBuff();

    // Disable echo
    SendESPcmd(UART_ID,"ATE0");
    ReadESPcmd(SEC(2));
    FlushCmdBuff();

    // Set WiFi Mode to Station 
    SendESPcmd(UART_ID,"AT+CWMODE=1");
    ReadESPcmd(SEC(2));
    FlushCmdBuff();

    // Set to Passive Mode to receive TCP info
    // AT+CIPRECVDATA=<size> | read the X amount of data from esp buffer
    // AT+CIPRECVLEN? | return the remaining  buffer size like this +CIPRECVLEN:636,0,0,0,0)
    // Also can read the actual size reading the +IPD value from "AT+CIPSEND" output: \r\n\r\nRecv 60 bytes\r\n\r\nSEND OK\r\n\r\n+IPD,636\r\nCLOSED\r\n
    SendESPcmd(UART_ID, "AT+CIPRECVMODE=1");
    ReadESPcmd(SEC(2));    
    FlushCmdBuff();

    ConnectESPWiFi(UART_ID, WiFiSSID, WiFiPSK);
    ReadESPcmd(SEC(10)); // WIFI DISCONNECT
    ReadESPcmd(SEC(10)); // WIFI CONNECTED
    ReadESPcmd(SEC(10)); // WIFI GOT IP
    ReadESPcmd(SEC(10)); // OK
    FlushCmdBuff();
//////////////////
// END CONFIGURE
//////////////////
   
    printf("\nDone\n");
    LED_OFF;

    while (true) {
        sleep_ms(1000);
    }
}