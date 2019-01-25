/*
 * main.c
 *
 *  Created on: March 2nd, 2018
 *      Author: John Stratoudakis
 */

#include "lwipopts.h"

#include "xparameters.h"

#include "helpers.h"

#include "lwipopts.h"
//#include "ten_gignetif.h"
#include "lwip/init.h"
#include "lwip/netif.h"

#include "lwip/init.h"

#include "lwip/debug.h"

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
//#include "lwip/timeouts.h"

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

    IP4_ADDR(&gw, 10, 0, 1, 1);
    IP4_ADDR(&ipaddr, 10, 0, 1, 101);
    IP4_ADDR(&netmask, 255, 0, 0, 0);

    u32 ret = init_all();

    printf("Good afternoon from mb_lwip, today is Friday, January 25th, 2019\n");
    printf("The time is now 2:49 PM \n");
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
//        if( XLlFifo_RxOccupancy(&fifo_2) )
//        {
//        	XGpio_DiscreteWrite(&gpio_1, 2, 0x10A);
//        	u32 recv_len_bytes;
//        	char buffer[MAX_FRAME_SIZE];
//
//        	recv_len_bytes = (XLlFifo_iRxGetLen(&fifo_2));
//        	if (recv_len_bytes > 0)
//        	{
//        		// Receive entire buffer amount
//        		XLlFifo_Read(&fifo_2, buffer, (recv_len_bytes));
//
//       			// Now package this up in to a UDP packet and send out
//        		udpecho_raw_send(buffer, recv_len_bytes);
//        	}
//    	}

//    	// Leaving this here as a "heartbeat"
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
