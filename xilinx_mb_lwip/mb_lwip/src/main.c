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

static ip_addr_t ipaddr, netmask, gw;

int main()
{
    struct netif netif;

    // TODO: You can manually override the ip address, gateway and netmask
    // here.  Improvement would be nice.
    IP4_ADDR(&gw, 10, 0, 1, 1);
    IP4_ADDR(&ipaddr, 10, 0, 1, 101);
    IP4_ADDR(&netmask, 255, 0, 0, 0);

    u32 ret = init_all();

    printf("Good afternoon from mb_lwip, today is Friday, February 8th, 2019\n");
    printf("The time is now 9:42 AM\n");
    printf("init_all() == %d\n", (int)ret);

    lwip_init();

    netif_add(&netif, &ipaddr, &netmask, &gw, NULL, tapif_init, ethernet_input);

    printf("CHECKPOINT-1\n");
    netif_set_default(&netif);

    udpecho_raw_init();
//    tcpecho_raw_init();

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
                u16 destPort = (buffer[6] << 8 & 0xFF00) | (buffer[7] & 0xFF);
                ip_addr_t destIp;
                destIp.addr = (buffer [2]) | (buffer [3] << 8) |
                		      (buffer [4] << 16) | (buffer [5] << 24);
                u16_t payload_len = (buffer[header_len + 1] & 0xFF >> 8) | (buffer[header_len + 2]);
                printf("header_len: %d\n", header_len);
                printf("session #: %d\n", buffer[1]);
                printf("destination address: %d.%d.%d.%d\n",
                            (destIp.addr >>  0) & 0xFF, (destIp.addr >>  8) & 0xFF,
               				(destIp.addr >> 16) & 0xFF, (destIp.addr >> 24) & 0xFF);
                printf("destination port: %d (0x%x)\n", destPort, destPort);
                printf("Dumping payload\n");
                printf("payload_len: %d\n", payload_len);
                char *payload = (char *) malloc( sizeof(char) * payload_len);
				for(i = 0; i < recv_len_bytes; i++)
				{
					// 22 is first byte of payload
					payload[i] = buffer[header_len + 3 + i];
					printf("0x%02x, ", (unsigned char)buffer[i]);
					if( (i+1) % 8 == 0)
						printf("\n");
				}
				printf("\n");

				printf("Calling udpecho_raw_send\n");
				udpecho_raw_send(destIp, destPort, payload, payload_len);
        	}
    	}

//    	// Check GPIO #1
//		u32 gpi_val_0 = XGpio_DiscreteRead(&gpio_1, 1);
//		if(gpi_val_0 != last_0) {
//			last_0 = gpi_val_0;
//			XGpio_DiscreteWrite(&gpio_1, 2, (last_0 + last_0));
//		}
//
//		// Check GPIO #2
//		u32 gpi_val_1 = XGpio_DiscreteRead(&gpio_2, 1);
//		if(gpi_val_1 != last_1) {
//			last_1 = gpi_val_1;
//			XGpio_DiscreteWrite(&gpio_2, 2, (last_1 + last_1));
//		}
    }

	return 0;
}
