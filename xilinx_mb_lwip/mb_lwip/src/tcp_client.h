/*
 * tcp_server.h
 *
 *  Created on: Feb 14, 2019
 *      Author: johns
 */

#ifndef SRC_TCP_CLIENT_H_
#define SRC_TCP_CLIENT_H_

#include "ipv4/lwip/ip_addr.h"
#include <xil_types.h>

void tcp_client_connect(ip_addr_t connect_ip, u16_t connect_port);
void tcp_client_send(char *payload, u16_t payload_len);
void tcp_client_receive(void);
void tcp_client_close(void);

#endif /* SRC_TCP_CLIENT_H_ */
