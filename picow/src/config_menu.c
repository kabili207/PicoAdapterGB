#include "config_menu.h"
#include "globals.h"

#include "pico/cyw43_arch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mobile_inet.h>

#include "gblink.h"

int main_parse_addr(struct mobile_addr *dest, char *argv){
    unsigned char ip[MOBILE_INET_PTON_MAXLEN];
    int rc = mobile_inet_pton(MOBILE_INET_PTON_ANY, argv, ip);

    struct mobile_addr4 *dest4 = (struct mobile_addr4 *)dest;
    struct mobile_addr6 *dest6 = (struct mobile_addr6 *)dest;
    switch (rc) {
        case MOBILE_INET_PTON_IPV4:
            dest4->type = MOBILE_ADDRTYPE_IPV4;
            memcpy(dest4->host, ip, sizeof(dest4->host));
            return 1;
            break;
        case MOBILE_INET_PTON_IPV6:
            dest6->type = MOBILE_ADDRTYPE_IPV6;
            memcpy(dest6->host, ip, sizeof(dest6->host));
            return 1;
            break;
        default:
            printf("Invalid IP address\n");
            return 0;
            break;
    }
}

void main_set_port(struct mobile_addr *dest, unsigned port){
    struct mobile_addr4 *dest4 = (struct mobile_addr4 *)dest;
    struct mobile_addr6 *dest6 = (struct mobile_addr6 *)dest;
    switch (dest->type) {
    case MOBILE_ADDRTYPE_IPV4:
        dest4->port = port;
        break;
    case MOBILE_ADDRTYPE_IPV6:
        dest6->port = port;
        break;
    default:
        printf("Invalid Port\n");
        break;
    }
}

static bool main_parse_hex(unsigned char *buf, char *str, unsigned size){
    unsigned char x = 0;
    for (unsigned i = 0; i < size * 2; i++) {
        char c = str[i];
        if (c >= '0' && c <= '9') c -= '0';
        else if (c >= 'A' && c <= 'F') c -= 'A' - 10;
        else if (c >= 'a' && c <= 'f') c -= 'a' - 10;
        else return false;

        x <<= 4;
        x |= c;

        if (i % 2 == 1) {
            buf[i / 2] = x;
            x = 0;
        }
    }
    return true;
}

void parse_addr_string(struct mobile_addr *src, char *dest){
    struct mobile_addr4 *addr4 = (struct mobile_addr4 *)src;
    struct mobile_addr6 *addr6 = (struct mobile_addr6 *)src;

    char tmpaddr[60] = {0};

    switch (src->type) {
        case MOBILE_ADDRTYPE_IPV4:            
            sprintf(tmpaddr,"%i.%i.%i.%i:%i\0", addr4->host[0], addr4->host[1], addr4->host[2], addr4->host[3], addr4->port);
            break;
        case MOBILE_ADDRTYPE_IPV6:
            sprintf(tmpaddr,"[%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx]:%i\0",
                    addr6->host[0],addr6->host[1], 
                    addr6->host[2],addr6->host[3], 
                    addr6->host[4],addr6->host[5], 
                    addr6->host[6],addr6->host[7], 
                    addr6->host[8],addr6->host[9], 
                    addr6->host[10],addr6->host[11], 
                    addr6->host[12],addr6->host[13], 
                    addr6->host[14],addr6->host[15], 
                    addr6->port);
            break;
        default:
            sprintf(tmpaddr,"No server defined.");
            break;
    }
    memcpy(dest,tmpaddr,sizeof(tmpaddr));
}

// Find a string into a buffer
bool FindCommand(char * buf, char * target){
    if(strstr(buf, target) != NULL){
        return true;
    }
    return false;
} 

void BootMenuConfig(void *user, char * wifissid, char * wifipass){
    struct mobile_user *mobile = (struct mobile_user *)user;

    int UserInput;
    printf("Press any key to enter in Setup Mode...\n");
    UserInput = getchar_timeout_us(SEC(5));
    if(UserInput != PICO_ERROR_TIMEOUT){
        char UserCMD[512] = {0};
        bool needSave = false;
        int haveDNS1 = 0;
        int haveDNS2 = 0;
        int haveRelaySrv = 0;

        char newSSID[28] = {0};
		char newPASS[28] = {0};
        
        memcpy(newSSID,wifissid,sizeof(newSSID));
        memcpy(newPASS,wifipass,sizeof(newPASS));

        //Libmobile Variables
        enum mobile_adapter_device device = MOBILE_ADAPTER_BLUE;
        bool device_unmetered = false;
        struct mobile_addr dns1 = {0};
        struct mobile_addr dns2 = {0};
        struct mobile_addr relay = {0};
        int dns_port = -1;
        bool relay_token_update = false;
        unsigned char *relay_token = NULL;
        unsigned char relay_token_buf[MOBILE_RELAY_TOKEN_SIZE];

        mobile_config_get_dns(mobile->adapter, &dns1, &dns2);
        mobile_config_get_relay(mobile->adapter, &relay);

        while(1){
            printf("Enter a command: \n");
            scanf("%512s",UserCMD);

            //Set the new WiFi SSID
            if(FindCommand(UserCMD,"WIFISSID=")){
                if(strlen(UserCMD)-9 > 0){
                    memset(newSSID,0x00,sizeof(newSSID));
                    memcpy(newSSID,UserCMD+9,strlen(UserCMD)-9);
                    printf("New WiFi SSID defined.\n");
                    needSave=true;  
                }else if(strlen(UserCMD)-9 == 0){
                    memcpy(newSSID,"WiFi_Network",12);
                    printf("Default WiFi SSID defined.\n");
                    needSave=true;  
                }else{
                    printf("Invalid parameter.\n");
                }

            //Set the new WiFi Password
            }else if(FindCommand(UserCMD,"WIFIPASS=")){
                if(strlen(UserCMD)-9 > 0){
                    memset(newPASS,0x00,sizeof(newPASS));
                    memcpy(newPASS,UserCMD+9,strlen(UserCMD)-9);
                    printf("New WiFi Password defined.\n");
                    needSave=true;
                }else if(strlen(UserCMD)-9 == 0){
                    memcpy(newPASS,"P@$$w0rd",8);
                    printf("Default WiFi Password defined.\n");
                    needSave=true;
                }else{
                    printf("Invalid parameter.\n");
                }
            
            //Set the new DNS1 for Libmobile
            }else if(FindCommand(UserCMD,"DNS1=")){
                if(strlen(UserCMD)-5 > 0){
                    char MAGB_DNS[60] = {0};
                    memcpy(MAGB_DNS,UserCMD+5,strlen(UserCMD)-5);
                    haveDNS1 = main_parse_addr(&dns1, MAGB_DNS);
                    if(haveDNS1 == 1){
                        printf("New DNS Server 1 defined.\n");
                        needSave=true;
                    } 
                }else if(strlen(UserCMD)-5 == 0){
                      haveDNS1 = -1;
                      printf("Default DNS Server 1 defined.\n");
                      needSave=true;
                }else{
                    printf("Invalid parameter.\n");
                }

            //Set the new DNS2 for Libmobile
            }else if(FindCommand(UserCMD,"DNS2=")){
                if(strlen(UserCMD)-5 > 0){
                    char MAGB_DNS[60] = {0};
                    memcpy(MAGB_DNS,UserCMD+5,strlen(UserCMD)-5);
                    haveDNS2 = main_parse_addr(&dns2, MAGB_DNS);
                    if(haveDNS2 == 1){
                        printf("New DNS Server 2 defined.\n");
                        needSave=true;
                    } 
                }else if(strlen(UserCMD)-5 == 0){
                      haveDNS2 = -1;
                      printf("Default DNS Server 2 defined.\n");
                      needSave=true;
                }else{
                    printf("Invalid parameter.\n");
                } 
            
            //Set the new DNS Port for Libmobile
            }else if(FindCommand(UserCMD,"DNSPORT=")){
                if(strlen(UserCMD)-8 > 0){
                    char dnsportstr[6] = {0};
                    memcpy(dnsportstr,UserCMD+8,strlen(UserCMD)-8);
                    if(atoi(dnsportstr) > 0){
                        dns_port = atoi(dnsportstr);
                        printf("New DNS Server port defined.\n");
                    }else{
                        printf("Invalid parameter.\n");
                    }
                    needSave=true;
                }else if(strlen(UserCMD)-8 == 0){
                    dns_port = MOBILE_DNS_PORT;
                    printf("Default DNS Server port defined.\n");
                    needSave=true;
                }else{
                    printf("Invalid parameter.\n");
                } 
            
            //Set the new Relay Server IP for Libmobile               
            }else if(FindCommand(UserCMD,"RELAYSERVER=")){
                if(strlen(UserCMD)-12 > 0){
                    char RELAY_SERVER[60] = {0};
                    memcpy(RELAY_SERVER,UserCMD+12,strlen(UserCMD)-12);
                    haveRelaySrv = main_parse_addr(&relay, RELAY_SERVER);
                    if(haveRelaySrv == 1){
                        main_set_port(&relay, MOBILE_DEFAULT_RELAY_PORT);
                        mobile_config_set_relay(mobile->adapter, &relay);
                        printf("New Relay Server Address defined.\n");
                        needSave=true;
                    } 
                }else if(strlen(UserCMD)-12 == 0){
                    mobile_config_set_relay(mobile->adapter, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE});
                    printf("Relay Server Address removed.\n");
                    needSave=true;
                }else{
                    printf("Invalid parameter.\n");
                } 

            //Set the new Relay Token for Libmobile                    
            }else if(FindCommand(UserCMD,"RELAYTOKEN=")){
                if(strlen(UserCMD)-11 > 0){
                    char RELAY_TOKEN[33] = {0};
                    memcpy(RELAY_TOKEN,UserCMD+11,32);
                    bool TokenOk = main_parse_hex(relay_token_buf, RELAY_TOKEN, sizeof(relay_token_buf));
                    if(!TokenOk){
                        printf("Invalid parameter.\n");
                    }else{
                        mobile_config_set_relay_token(mobile->adapter, relay_token_buf);
                        printf("New Relay Token defined.\n");
                        needSave=true;
                    }
                }else if(strlen(UserCMD)-11 == 0){
                    mobile_config_set_relay_token(mobile->adapter, NULL);
                    printf("Relay Token removed.\n");
                    needSave=true;
                }else{
                    printf("Invalid parameter.\n");
                } 

            //Set the new P2P Port for Libmobile 
            }else if(FindCommand(UserCMD,"P2PPORT=")){
                if(strlen(UserCMD)-8 > 0){
                    char p2pportstr[6] = {0};                    
                    memcpy(p2pportstr,UserCMD+8,strlen(UserCMD)-8);
                    if(atoi(p2pportstr) > 0){
                        mobile_config_set_p2p_port(mobile->adapter, atoi(p2pportstr));
                        printf("New P2P Port defined.\n");
                    }else{
                        printf("Invalid parameter.\n");
                    }
                    needSave=true;
                }else if(strlen(UserCMD)-8 == 0){
                    mobile_config_set_p2p_port(mobile->adapter, MOBILE_DEFAULT_P2P_PORT);
                    printf("Default P2P Port defined.\n");
                    needSave=true;
                }else{
                    printf("Invalid parameter.\n");
                } 
            
            //Set the new Unmetered setting for Libmobile 
            }else if(FindCommand(UserCMD,"UNMETERED=")){
                if(strlen(UserCMD)-10 == 1){
                    bool unmeteredcheck=false;
                    switch (UserCMD[10]){
                        case '0':
                            unmeteredcheck=false;
                            break;
                        case '1':
                            unmeteredcheck=true;
                            break;
                        default:
                            printf("Invalid parameter. Applying default value.\n");
                            break;
                    }
                    mobile_config_set_device(mobile->adapter, device, unmeteredcheck);
                    printf("New Unmetered value defined.\n");
                    needSave=true;
                }else if(strlen(UserCMD)-10 == 0){
                    mobile_config_set_device(mobile->adapter, device, false);
                    printf("Default Unmetered value defined.\n");
                    needSave=true;
                }else{
                    printf("Invalid parameter.\n");
                } 
            
            //Format the entire EEPROM, if necessary
            }else if(FindCommand(UserCMD,"FORMAT_EEPROM")){
                printf("Formatting...\n");
                memset(mobile->config_eeprom,0x00,sizeof(mobile->config_eeprom));

                mobile_config_set_dns(mobile->adapter, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE}, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE});
                mobile_config_set_relay(mobile->adapter, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE});
                mobile_config_set_relay_token(mobile->adapter, NULL);
                mobile_config_set_p2p_port(mobile->adapter, MOBILE_DEFAULT_P2P_PORT);
                mobile_config_set_device(mobile->adapter, device, false);
                
                mobile_config_save(mobile->adapter);
                busy_wait_ms(2*1000);
                
                haveDNS1 = 0;
                haveDNS2 = 0;
                haveRelaySrv = 0;

                mobile_config_get_dns(mobile->adapter, &dns1, &dns2);
                mobile_config_get_relay(mobile->adapter, &relay);
                
                memset(newSSID,0x00,sizeof(newSSID));
                memset(newPASS,0x00,sizeof(newPASS));
                strcpy(newSSID,"WiFi_Network");
                strcpy(newPASS,"P@$$w0rd");

                RefreshConfigBuff(mobile->config_eeprom,newSSID,newPASS);

                printf("Device formatted!\n");
                needSave=true;

            //Exit from Menu
            }else if(FindCommand(UserCMD,"EXIT")){
                break;

            // Shows the actual device configuration
            }else if(FindCommand(UserCMD,"SHOW_CONFIG")){
                printf("NETWORK SETTINGS\n");
                printf("WiFi SSID: %s\n",newSSID);
                printf("WiFi Password: %s\n",newPASS);
                printf("\n");
                printf("ADAPTER SETTINGS\n");

                char tmpSrv[60] = {0};
                parse_addr_string(&dns1,tmpSrv);
                printf("DNS Server 1: %s\n", tmpSrv);
                parse_addr_string(&dns2,tmpSrv);
                printf("DNS Server 2: %s\n", tmpSrv);
                parse_addr_string(&relay,tmpSrv);
                printf("Relay Server: %s\n", tmpSrv);

                char tmpToken[16] = {0};
                mobile_config_get_relay_token(mobile->adapter,tmpToken);
                printf("Relay Token: ");
                for (int i = 0; i < sizeof(tmpToken); i++){
                    printf("%02hhX", tmpToken[i]);
                }
                printf("\n");

                unsigned tmpPort = 0;
                mobile_config_get_p2p_port(mobile->adapter,&tmpPort);
                printf("P2P Port: %i\n", tmpPort);

                bool tmpUnmet = false;
                mobile_config_get_device(mobile->adapter,NULL,&tmpUnmet);
                printf("Is Unmetered: %s\n", tmpUnmet == true ? "Yes":"No");
                printf("\n");

            }else if(FindCommand(UserCMD,"HELP")){
                printf("Command Sintax: <COMMAND>=<VALUE>\n");
                printf("To reset/clear a parameter, leave the <VALUE> blank, for example: WIFISSID=\n");
                printf("All commands are Case-Sensitive\n\n");
                printf("Command List:\n");
                printf("WIFISSID      | Set the SSID to use to connect.\n");
                printf("WIFIPASS      | Set the password from the WiFi network to use to connect.\n");
                printf("DNS1          | Set primary DNS that the adapter will use to parse the nameservers.\n");
                printf("DNS2          | Set secondary DNS that the adapter will use to parse the nameservers.\n");
                printf("DNSPORT       | Set a custom DNS port to use with the DNS servers.\n");
                printf("RELAYSERVER   | Set a Relay Server that will be use during P2P communications.\n");
                printf("RELAYTOKEN    | Set a Relay Token that will be used on Relay Server to receive a valid number to use during P2P communications.\n");
                printf("P2PPORT       | Set a custom P2P port to use during P2P communications (Local Network only).\n");
                printf("UNMETERED     | Set if the device will be Unmetered (useful for Pokemon Crystal). Only accept 1 (true) or 0 (false).\n\n");

                printf("Special commands (just enter the command, without =<VALUE>):\n");
                printf("FORMAT_EEPROM | Format the eeprom, if necessary.\n");
                printf("SHOW_CONFIG   | Show the actual device configuration.\n");
                printf("EXIT          | Quit from Config Mode and Save the new values. If you change some value, the device will reboot.\n");
                printf("HELP          | Show this command list on screen.\n\n");
                
            //Generic error return
            }else{
                printf("Invalid command.\n");
            }
        }

        if(needSave){
            printf("Saving new configs...\n");
            //Parsing new DNS Server
            if (haveDNS1 == 1 && haveDNS2 == 1){
                if(dns_port > 0){
                    main_set_port(&dns1, dns_port);
                    main_set_port(&dns2, dns_port);
                }else{
                    main_set_port(&dns1, MOBILE_DNS_PORT);
                    main_set_port(&dns2, MOBILE_DNS_PORT);
                }
                mobile_config_set_dns(mobile->adapter, &dns1, &dns2);
            }else if (haveDNS1 == -1 && haveDNS2 == -1){
                mobile_config_set_dns(mobile->adapter, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE}, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE});
            }else if (haveDNS1 == 1 && haveDNS2 == -1){
                if(dns_port > 0){
                    main_set_port(&dns1, dns_port);
                }else{
                    main_set_port(&dns1, MOBILE_DNS_PORT);
                }
                mobile_config_set_dns(mobile->adapter, &dns1, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE});
            }else if (haveDNS1 == -1 && haveDNS2 == 1){
                if(dns_port > 0){
                    main_set_port(&dns2, dns_port);
                }else{
                    main_set_port(&dns2, MOBILE_DNS_PORT);
                }
                mobile_config_set_dns(mobile->adapter, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE}, &dns2);
            }else if (haveDNS1 == 0 || haveDNS2 == 0){
                if (haveDNS1 == 0 && haveDNS2 == -1){
                    if(dns_port > 0){
                        main_set_port(&dns1, dns_port);
                    }else{
                        main_set_port(&dns1, MOBILE_DNS_PORT);
                    }
                    mobile_config_set_dns(mobile->adapter, &dns1, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE});
                }else if (haveDNS1 == -1 && haveDNS2 == 0){
                    if(dns_port > 0){
                        main_set_port(&dns2, dns_port);
                    }else{
                        main_set_port(&dns2, MOBILE_DNS_PORT);
                    }
                    mobile_config_set_dns(mobile->adapter, &(struct mobile_addr){.type=MOBILE_ADDRTYPE_NONE}, &dns2);
                }else{
                    if(dns_port > 0){
                        main_set_port(&dns1, dns_port);
                        main_set_port(&dns2, dns_port);
                    }else{
                        main_set_port(&dns1, MOBILE_DNS_PORT);
                        main_set_port(&dns2, MOBILE_DNS_PORT);
                    }
                    mobile_config_set_dns(mobile->adapter, &dns1, &dns2);
                }
            }

            //Save new Configs
            mobile_config_save(mobile->adapter);
            busy_wait_ms(2*1000);
            RefreshConfigBuff(mobile->config_eeprom,newSSID,newPASS);

            printf("Please reboot the device...\n");
            while(true){
                LED_ON;
                busy_wait_ms(300);
                LED_OFF;
                busy_wait_ms(300);
            }
        }
    }
    printf("Continuing initialization...\n");
}