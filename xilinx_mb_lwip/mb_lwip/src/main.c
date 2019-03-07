/*
 * main.c
 *
 *  Created on: March 2nd, 2018
 *      Author: John Stratoudakis
 */

#include "lwipopts.h"

#include "xparameters.h"

#include "helpers.h"

#include "lwip/init.h"
#include "lwip/netif.h"

#include "lwip/init.h"

#include "lwip/debug.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"

#include "lwip/stats.h"

#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/ip_frag.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "tapif.h"
#include "netif/etharp.h"

#include "udpecho_raw.h"
#include "tcpecho_raw.h"
#include "tcp_client.h"

static ip_addr_t ipaddr, netmask, gw;

int main()
{
    struct netif netif;

    ip_addr_t connect_ip;
    u16_t connect_port = 35312;

    IP4_ADDR(&gw, 10, 0, 1, 1);
    IP4_ADDR(&ipaddr, 10, 0, 1, 101);
    IP4_ADDR(&connect_ip, 10, 0, 1, 100);
    IP4_ADDR(&netmask, 255, 0, 0, 0);

    u32 ret = init_all();

    printf("Good afternoon from mb_lwip, today is Thursday, March 7th, 2019\n");
    printf("The time is now 11:52 AM\n");
    printf("init_all() == %d\n", (int)ret);

    printf("Parameters:\n");
    printf("IP_DEBUG=%d\n", IP_DEBUG);
    printf("LWIP_DBG_TYPES_ON=%d\n", LWIP_DBG_TYPES_ON);
    printf("LWIP_DBG_ON=%d\n", LWIP_DBG_ON);
    printf("LWIP_DBG_MASK_LEVEL=%d\n", LWIP_DBG_MASK_LEVEL);
    printf("LWIP_DBG_MIN_LEVEL=%d\n", LWIP_DBG_MIN_LEVEL);
    printf("LWIP_DBG_HALT=%d\n", LWIP_DBG_HALT);

    LWIP_DEBUGF(IP_DEBUG, ("LWIP_DEBUGF_TEST\n"));

    lwip_init();

    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init, ethernet_input);

    printf("CHECKPOINT-1\n");
    netif_set_default(&netif);

    //udpecho_raw_init(connect_port);
    //tcpecho_raw_init(port);

    netif_set_up(&netif);
    printf("CHECKPOINT-2\n");

    while(1)
    {
    	// Check for new data to send in to the TCP/ip stack
        tapif_select(&netif);

        // Check for new data to send from LabVIEW
        if( XLlFifo_RxOccupancy(&fifo_2) )
        {
        	u32 recv_len_bytes;
        	static char buffer[MAX_FRAME_SIZE];

        	recv_len_bytes = (XLlFifo_iRxGetLen(&fifo_2));
        	if (recv_len_bytes > 0)
        	{
        		int i;
        		printf("TX_PACKET - reading in %lu bytes\n", recv_len_bytes);
        		// Receive entire buffer amount
        		XLlFifo_Read(&fifo_2, buffer, (recv_len_bytes));

        		printf("Dumping packet\n  ");
                for (i = 0; i < recv_len_bytes; i++)
                {
                    printf("0x%02x, ", (unsigned char)buffer[i]);
                    if( (i+1) % 8 == 0)
                        printf("\n  ");
                }
                printf("\nEnd of dump\n");

                printf("Identified parameters\n");
                u8 header_len = buffer[0];
                u8 session_id = buffer[1];
                u8 command = buffer[8];
                u16 destPort = (buffer[6] << 8 & 0xFF00) | (buffer[7] & 0xFF);
                ip_addr_t destIp;
                destIp.addr = (buffer [2]) | (buffer [3] << 8) |
                		      (buffer [4] << 16) | (buffer [5] << 24);
                u16_t payload_len = (buffer[header_len + 1] & 0xFF >> 8) | (buffer[header_len + 2]);
                printf("header_len: %d\n", header_len);
                printf("session #: %d\n", session_id);
                printf("command: %d\n", command);
                printf("destination address: %d.%d.%d.%d\n",
                            (destIp.addr >>  0) & 0xFF, (destIp.addr >>  8) & 0xFF,
               				(destIp.addr >> 16) & 0xFF, (destIp.addr >> 24) & 0xFF);
                printf("destination port: %d (0x%x)\n", destPort, destPort);
                printf("Dumping payload\n");
                printf("payload_len: %d\n", payload_len);
                char *payload = (char *) malloc( sizeof(char) * payload_len);
				for(i = 0; i < payload_len; i++)
				{
					// header_len is 16
					// header_len length is 1
					// payload_len length is 2
					payload[i] = buffer[header_len + 3 + i];
					printf("0x%02x, ", (unsigned char)payload[i]);
					if( (i+1) % 8 == 0)
						printf("\n");
				}
				printf("\n");

				printf("Passing command down\n");
				if(session_id == 1) {
					printf("Sending to UDP Session (session_id = 1)\n");
					udpecho_raw_send(destIp, destPort, payload, payload_len);
				} else if (session_id == 2) {
					printf("Sending to TCP Session (session_id = 2)\n");
					if(command == 0) {
						tcp_client_connect(connect_ip, connect_port);
					} else if(command == 1) {
						tcp_client_send(payload, payload_len);
					} else if(command == 2) {
						// Not implemented yet
					} else if(command == 3) {
						tcp_client_close();
					}
				}
        	}
    	}
    }

	return 0;
}
