/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "helpers.h"

//#include <fcntl.h>
//#include <stdlib.h>
//#include <stdio.h>
#include <unistd.h>
#include <string.h>
//#include <sys/ioctl.h>
//#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
//#include <sys/uio.h>
//#include <sys/socket.h>

#include "lwip/opt.h"

#include "lwip/debug.h"
#include "lwip/def.h"
#include "lwip/ip.h"
#include "lwip/mem.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
//#include "lwip/timeouts.h"
#include "netif/etharp.h"
//#include "lwip/ethip6.h"

#if defined(LWIP_DEBUG) && defined(LWIP_TCPDUMP)
#include "netif/tcpdump.h"
#endif /* LWIP_DEBUG && LWIP_TCPDUMP */

#include "tapif.h"

#define IFCONFIG_BIN "/sbin/ifconfig "

#if defined(LWIP_UNIX_LINUX)
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
/*
 * Creating a tap interface requires special privileges. If the interfaces
 * is created in advance with `tunctl -u <user>` it can be opened as a regular
 * user. The network must already be configured. If DEVTAP_IF is defined it
 * will be opened instead of creating a new tap device.
 *
 * You can also use PRECONFIGURED_TAPIF environment variable to do so.
 */
#ifndef DEVTAP_DEFAULT_IF
#define DEVTAP_DEFAULT_IF "tap0"
#endif
#ifndef DEVTAP
#define DEVTAP "/dev/net/tun"
#endif
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tap0 inet %d.%d.%d.%d " NETMASK_ARGS
#elif defined(LWIP_UNIX_OPENBSD)
#define DEVTAP "/dev/tun0"
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tun0 inet %d.%d.%d.%d " NETMASK_ARGS " link0"
#else /* others */
#define DEVTAP "/dev/tap0"
#define NETMASK_ARGS "netmask %d.%d.%d.%d"
#define IFCONFIG_ARGS "tap0 inet %d.%d.%d.%d " NETMASK_ARGS
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 't'
#define IFNAME1 'p'

#ifndef TAPIF_DEBUG
#define TAPIF_DEBUG LWIP_DBG_OFF
#endif


struct tapif {
	/* Add whatever per-interface state that is needed here. */
	int fd;
};

/* Forward declarations. */
static void tapif_input(struct netif *netif);
#if !NO_SYS
static void tapif_thread(void *arg);
#endif /* !NO_SYS */

// Called by tapif_init
static void low_level_init(struct netif *netif)
{
    printf(" - low_level_init.start\n");
//	struct tapif *tapif;

	int ret;
	static char buf[1024];

//	char *preconfigured_tapif = getenv("PRECONFIGURED_TAPIF");
//	tapif = (struct tapif *) netif->state;

	/* Obtain MAC address from network interface. */

	/* (We just fake an address...) */
	netif->hwaddr[0] = 0x00;
	netif->hwaddr[1] = 0x01;
	netif->hwaddr[2] = 0x02;
	netif->hwaddr[3] = 0x03;
	netif->hwaddr[4] = 0x04;
	netif->hwaddr[5] = 0x05;
	netif->hwaddr_len = 6;

	/* device capabilities */
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;

	// What should we do here? Write a register in the FPGA?
	// Send a signal to turn on the PHY?
	// Or just nothing?

	printf("    calling netif_set_link_up\n");
	netif_set_link_up(netif);

//  if (preconfigured_tapif == NULL) {
//	  XGpio_DiscreteWrite(&gpio_2, 2, 0xCCCCC00A);
//#if LWIP_IPV4
//    snprintf(buf, 1024, IFCONFIG_BIN IFCONFIG_ARGS,
//             ip4_addr1(netif_ip4_gw(netif)),
//             ip4_addr2(netif_ip4_gw(netif)),
//             ip4_addr3(netif_ip4_gw(netif)),
//             ip4_addr4(netif_ip4_gw(netif))
//#ifdef NETMASK_ARGS
//             ,
//             ip4_addr1(netif_ip4_netmask(netif)),
//             ip4_addr2(netif_ip4_netmask(netif)),
//             ip4_addr3(netif_ip4_netmask(netif)),
//             ip4_addr4(netif_ip4_netmask(netif))
//#endif /* NETMASK_ARGS */
//             );
//
//    XGpio_DiscreteWrite(&gpio_2, 2, 0xCCCCC00B);
//    LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_init: system(\"%s\");\n", buf));
//    XGpio_DiscreteWrite(&gpio_2, 2, 0xCCCCC00C);
////    ret = system(buf);
////    XGpio_DiscreteWrite(&gpio_2, 2, 0xCCCCC00D);
////    if (ret < 0) {
////      perror("ifconfig failed");
////      exit(1);
////    }
////    if (ret != 0) {
////      printf("ifconfig returned %d\n", ret);
////    }
////#else /* LWIP_IPV4 */
////    perror("todo: support IPv6 support for non-preconfigured tapif");
////    exit(1);
//#endif /* LWIP_IPV4 */
//  }
	print(" - low_level_init.end\n");
}

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
/*--------------------------------------------------------------------------*/
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    static char buf[1514];
    static int outLen = 0;
    int i;
    const int frame_alignment = 64;

    printf("============================================================\n");
    printf(" low_level_output:\n");
    printf("------------------------------------------------------------\n");

    if(p == NULL)
    {
        printf("p == NULL\n");
        return ERR_OK;
    }

    outLen = p->tot_len;
    printf(" Sending out a packet of length: %d\n", outLen);
    if( outLen % frame_alignment != 0)
    {
        int rem = outLen % frame_alignment;
        printf("  -- remainder of %d bytes --\n", rem);

        outLen += (frame_alignment - rem);
        printf("  -- new length %d bytes --\n", outLen);
    }

    // Add 40 bytes of padding as a test
//    outLen += 40;
//    printf("  -- with padding %d bytes --\n", outLen);

    for(i=0; i<outLen; i++) {
    	buf[i] = 0;
    }

    /* initiate transfer(); */
    pbuf_copy_partial(p, buf, p->tot_len, 0);
    printf(" After calling pbuf_copy_partial\n");

    // TOOD: Make sure output is rounded up to the nearest 32-bit word
    // ADD zeros
    // TODO: Try with 8 byte alignment, the ideal
    // transfer p->tot_len from buf to FPGA
    if (XLlFifo_iTxVacancy(&fifo_1))
    {
        printf(" There exists a vacancy in fifo #1\n  ");

        int i = 0;
        printf(" Before byte re-ordering outLen=%d\n  ", outLen);
        for ( i = 0; i < outLen; i++)
        {
            printf("0x%02x, ", (unsigned char)buf[i]);
            if( (i+1) % 8 == 0)
                printf("\n  ");
            }
            char temp;
            for(i = 0; i < outLen; i += 4)
            {
                // 0, 1, 2, 3
                // 1, 0, 3, 2
                // 3, 2, 1, 0
                temp = buf[i + 0];
                buf[i + 0] = buf[i + 3];
                buf[i + 3] = temp;

                temp = buf[i + 1];
                buf[i + 1] = buf[i + 2];
                buf[i + 2] = temp;
            }

            printf("\n\n");
            printf(" After byte re-ordering, outLen=%d\n  ", outLen);
            for ( i = 0; i < outLen; i++)
            {
                printf("0x%02x, ", (unsigned char)buf[i]);
                if( (i+1) % 8 == 0)
                    printf("\n  ");
            }

            XLlFifo_Write(&fifo_1, buf, outLen);
            XLlFifo_iTxSetLen(&fifo_1, outLen);
        } else {
            printf("No transmit vacancy?\n");
	}

    printf("\n============================================================\n");

	return ERR_OK;
}

/*--------------------------------------------------------------------------*/
int tapif_select(struct netif *netif)
{
    int ret = 0;
    int i = 0;
    int dump = 0;

    if( XLlFifo_RxOccupancy(&fifo_1) )
    {
        u32 recv_len_bytes;
        static char buffer[MAX_FRAME_SIZE];
        recv_len_bytes = (XLlFifo_iRxGetLen(&fifo_1));

        printf("============================================================\n");
        printf(" tapif_select:\n");
        printf(" New Packet received of size: %d\n", (int)recv_len_bytes);
        printf("------------------------------------------------------------\n");

        if (recv_len_bytes > 0)
        {
            // Receive entire buffer amount
            XLlFifo_Read(&fifo_1, buffer, (recv_len_bytes));

            if(buffer[15] == 0x80 && buffer[14] == 0x0) {
            	printf("Dumping frame because EtherType is IPv4\n");
            	dump = 1;
            }
            // Removing last 4 bytes, which is an indicator from the FPGA if the frame is good or bad
            // 0x1 = Bad Frame
            // 0x2 = Good Frame
            recv_len_bytes -= 4;

            printf(" [BEFORE] recv_len_bytes %d\n", (int)recv_len_bytes);

            if(dump) {
            	printf("   beforePacket = [\n      ");
            	for (i = 0; i < recv_len_bytes; i++)
            	{
                	printf("0x%02x, ", (unsigned char)buffer[i]);
                	if( (i+1) % 8 == 0)
                		printf("\n      ");
            	}
            	printf("\n ]\n");
            }

            static char temp;
            for(i = 0; i < recv_len_bytes; i += 4)
            {
                temp = buffer[i + 0];
                buffer[i + 0] = buffer[i + 3];
                buffer[i + 3] = temp;

                temp = buffer[i + 1];
                buffer[i + 1] = buffer[i + 2];
                buffer[i + 2] = temp;
            }
            // todo: fix up remainder
            if( recv_len_bytes % 4 != 0)
            {
                int rem = recv_len_bytes % 4;
                printf("   -- remainder of %d bytes --\n", rem);
            }

            printf(" [AFTER] recv_len_bytes %d\n", (int)recv_len_bytes);
            printf(" [INFO] EtherType: 0x%x 0x%x\n", (unsigned char)buffer[12], (unsigned char)buffer[13]);
            if(buffer[12] == 0x8 && buffer[13] == 0x00){
            	printf(" [INFO] Protocol: 0x%x\n", (unsigned char)buffer[23]);
            }

            if(dump) {
            	printf("   afterPacket = [\n      ");
            	for (i = 0; i < recv_len_bytes; i++)
            	{
                	printf("0x%02x, ", (unsigned char)buffer[i]);
                	if( (i+1) % 8 == 0)
                		printf("\n      ");
            	}
            	printf(" ]\n");
            }

            struct pbuf *p;
            /* We allocate a pbuf chain of pbufs from the pool. */
            p = pbuf_alloc(PBUF_RAW, recv_len_bytes, PBUF_POOL);
            if (p != NULL) {
                pbuf_take(p, buffer, recv_len_bytes);
                /* acknowledge that packet has been read(); */
                printf("pbuf_take()\n");
                printf(" Calling netif->input\n");
                netif->input(p, netif);
                pbuf_free(p);
                printf("After pbuf_free()\n");
            } else {
                /* drop packet(); */
            	printf("drop_packet()\n");
                LWIP_DEBUGF(NETIF_DEBUG, ("tapif_input: could not allocate pbuf\n"));
            }
        }
        printf("EE==========================================================\n");
    }

	return ret;
}

/*-----------------------------------------------------------------------------------*/
/*
 * low_level_input():
 *
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 */
/*-----------------------------------------------------------------------------------*/
static struct pbuf *low_level_input(struct netif *netif)
{
	struct pbuf *p;
//	u16_t len;
//	ssize_t readlen;
	size_t i;
	static char buf[1514];

	/* Obtain the size of the packet and put it into the "len"
	 variable. */
//	readlen = 0;
	if( XLlFifo_RxOccupancy(&fifo_1) )
	{
		printf("============================================================\n");
		printf(" low_level_input:\n");
		printf("------------------------------------------------------------\n");

		XGpio_DiscreteWrite(&gpio_1, 2, 0x10A);
		u32 recv_len_bytes = (XLlFifo_iRxGetLen(&fifo_1));
		if (recv_len_bytes > 0)
		{
			printf("   New Packet received of size: %d\n", (int)recv_len_bytes);
			// Receive entire buffer amount
			XLlFifo_Read(&fifo_2, buf, (recv_len_bytes));

			for (i = 0; i < recv_len_bytes; i++)
			{
				XGpio_DiscreteWrite(&gpio_2, 2, buf[i]);
				printf("0x%02x, ", (unsigned char)buf[i]);
				if( (i+1) % 8 == 0)
					printf("\n  ");
			}
//			p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
//			if (p != NULL)
//			{
//				print("    pbuf_take\n");
//				pbuf_take(p, buf, len);
//				/* acknowledge that packet has been read(); */
//			}
			// Now package this up in to a UDP packet and send out
	//		udpecho_raw_send(buffer, recv_len_bytes);
		}
		printf("E_low_level_input============================================================\n");
	}

	return p;
}

/*-----------------------------------------------------------------------------------*/
/*
 * tapif_input():
 *
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface.
 *
 */
/*-----------------------------------------------------------------------------------*/
static void tapif_input(struct netif *netif)
{
	struct pbuf *p = low_level_input(netif);

	printf(" - tapif_input\n");

	if (p == NULL) {
#if LINK_STATS
		LINK_STATS_INC(link.recv);
#endif /* LINK_STATS */
		LWIP_DEBUGF(TAPIF_DEBUG, ("tapif_input: low_level_input returned NULL\n"));
		return;
	}

	if (netif->input(p, netif) != ERR_OK) {
		LWIP_DEBUGF(NETIF_DEBUG, ("tapif_input: netif input error\n"));
		pbuf_free(p);
	}
}
/*-----------------------------------------------------------------------------------*/
/*
 * tapif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
/*-----------------------------------------------------------------------------------*/
err_t tapif_init(struct netif *netif)
{
	struct tapif *tapif = (struct tapif *) mem_malloc(sizeof(struct tapif));

	printf(" - tapif_init.start\n");
	if (tapif == NULL)
	{
		printf("    tapif_init: out of memory for tapif\n");
		LWIP_DEBUGF(NETIF_DEBUG, ("tapif_init: out of memory for tapif\n"));
		return ERR_MEM;
	}
	netif->state = tapif;
//  MIB2_INIT_NETIF(netif, snmp_ifType_other, 100000000);

	netif->name[0] = IFNAME0;
	netif->name[1] = IFNAME1;
	netif->output = etharp_output;
	netif->linkoutput = low_level_output;
	netif->mtu = 1500;

	low_level_init(netif);

	printf(" - tapif_init.end\n");

	return ERR_OK;
}

