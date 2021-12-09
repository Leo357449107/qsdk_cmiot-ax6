/******************************************************************************

			L I B R M N E T C T L . C

Copyright (c) 2013-2015, 2018, 2020-2021
The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
	* Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.
	* Redistributions in binary form must reproduce the above
	  copyright notice, this list of conditions and the following
	  disclaimer in the documentation and/or other materials provided
	  with the distribution.
	* Neither the name of The Linux Foundation nor the names of its
	  contributors may be used to endorse or promote products derived
	  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

/*!
* @file    librmnetctl.c
* @brief   rmnet control API's implementation file
*/

/*===========================================================================
			INCLUDE FILES
===========================================================================*/

#include <sys/socket.h>
#include <stdint.h>
#include <linux/netlink.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/rtnetlink.h>
#include <linux/gen_stats.h>
#include <net/if.h>
#include <asm/types.h>
#include <linux/rmnet_data.h>
#include "librmnetctl_hndl.h"
#include "librmnetctl.h"

#ifdef USE_GLIB
#include <glib.h>
#define strlcpy g_strlcpy
#endif

#define RMNETCTL_SOCK_FLAG 0
#define ROOT_USER_ID 0
#define MIN_VALID_PROCESS_ID 0
#define MIN_VALID_SOCKET_FD 0
#define KERNEL_PROCESS_ID 0
#define UNICAST 0
#define MAX_BUF_SIZE sizeof(struct nlmsghdr) + sizeof(struct rmnet_nl_msg_s)
#define INGRESS_FLAGS_MASK   (RMNET_INGRESS_FIX_ETHERNET | \
			      RMNET_INGRESS_FORMAT_MAP | \
			      RMNET_INGRESS_FORMAT_DEAGGREGATION | \
			      RMNET_INGRESS_FORMAT_DEMUXING | \
			      RMNET_INGRESS_FORMAT_MAP_COMMANDS | \
			      RMNET_INGRESS_FORMAT_MAP_CKSUMV3 | \
			      RMNET_INGRESS_FORMAT_MAP_CKSUMV4)
#define EGRESS_FLAGS_MASK    (RMNET_EGRESS_FORMAT__RESERVED__ | \
			      RMNET_EGRESS_FORMAT_MAP | \
			      RMNET_EGRESS_FORMAT_AGGREGATION | \
			      RMNET_EGRESS_FORMAT_MUXING | \
			      RMNET_EGRESS_FORMAT_MAP_CKSUMV3 | \
			      RMNET_EGRESS_FORMAT_MAP_CKSUMV4)

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define NLMSG_TAIL(nmsg) \
    ((struct rtattr *) (((char *)(nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

#define NLMSG_DATA_SIZE  500

#define CHECK_MEMSCPY(x) ({if (x < 0 ){return RMNETCTL_LIB_ERR;}})

struct nlmsg {
	struct nlmsghdr nl_addr;
	struct ifinfomsg ifmsg;
	char data[NLMSG_DATA_SIZE];
};

struct rmnetctl_uplink_params {
	uint16_t byte_count;
	uint8_t packet_count;
	uint8_t features;
	uint32_t time_limit;
};

/* IFLA Attributes for the RT RmNet driver */
enum {
	RMNETCTL_IFLA_UNSPEC,
	RMNETCTL_IFLA_MUX_ID,
	RMNETCTL_IFLA_FLAGS,
	RMNETCTL_IFLA_DFC_QOS,
	RMNETCTL_IFLA_UPLINK_PARAMS,
	RMNETCTL_IFLA_NSS_OFFLOAD,
	__RMNETCTL_IFLA_MAX,
};

/* Flow message types sent to DFC driver */
enum {
	/* Activate flow */
	RMNET_FLOW_MSG_ACTIVATE = 1,
	/* Delete flow */
	RMNET_FLOW_MSG_DEACTIVATE = 2,
	/* Legacy flow control */
	RMNET_FLOW_MSG_CONTROL = 3,
	/* Flow up */
	RMNET_FLOW_MSG_UP = 4,
	/* Flow down */
	RMNET_FLOW_MSG_DOWN = 5,
	/* Change ACK scaling */
	RMNET_FLOW_MSG_QMI_SCALE = 6,
	/* Change powersave workqueue polling freq */
	RMNET_FLOW_MSG_WDA_FREQ = 7,
	/* Add a filter */
	RMNET_FLOW_MSG_ADD_FILTER = 10,
	/* Remove filters */
	RMNET_FLOW_MSG_REMOVE_FILTERS = 11,
};

/* 0 reserved, 1-15 for data, 16-30 for acks */
#define RMNETCTL_NUM_TX_QUEUES 31

/* This needs to be hardcoded here because some legacy linux systems
 * do not have this definition
 */
#define RMNET_IFLA_NUM_TX_QUEUES 31

/*===========================================================================
			LOCAL FUNCTION DEFINITIONS
===========================================================================*/
/* Helper functions
 * @brief helper function to implement a secure memcpy
 * @details take source and destination buffer size into
 *          considerations before copying
 * @param dst destination buffer
 * @param dst_size size of destination buffer
 * @param src source buffer
 * @param src_size size of source buffer
 * @return size of the smallest of two buffer
 */

static inline size_t memscpy(void *dst, size_t dst_size, const void *src,
			     size_t src_size) {
	size_t min_size = dst_size < src_size ? dst_size : src_size;
	memcpy(dst, src, min_size);
	return min_size;
}

/*
* @brief helper function to implement a secure memcpy
 * for a concatenating buffer where offset is kept
 * track of
 * @details take source and destination buffer size into
 *          considerations before copying
 * @param dst destination buffer
 * @param dst_size pointer used to decrement
 * @param src source buffer
 * @param src_size size of source buffer
 * @return size of the remaining buffer
 */


static inline int  memscpy_repeat(void* dst, size_t *dst_size,
	const void* src, size_t src_size)
{
	if( !dst_size || *dst_size <= src_size || !dst || !src)
		return RMNETCTL_LIB_ERR;
	else {
		*dst_size -= memscpy(dst, *dst_size, src, src_size);
	}
	return *dst_size;
}


/*!
* @brief Synchronous method to send and receive messages to and from the kernel
* using  netlink sockets
* @details Increments the transaction id for each message sent to the kernel.
* Sends the netlink message to the kernel and receives the response from the
* kernel.
* @param *hndl RmNet handle for this transaction
* @param request Message to be sent to the kernel
* @param response Message received from the kernel
* @return RMNETCTL_API_SUCCESS if successfully able to send and receive message
* from the kernel
* @return RMNETCTL_API_ERR_HNDL_INVALID if RmNet handle for the transaction was
* NULL
* @return RMNETCTL_API_ERR_REQUEST_NULL not enough memory to create buffer for
* sending the message
* @return RMNETCTL_API_ERR_MESSAGE_SEND if could not send the message to kernel
* @return RMNETCTL_API_ERR_MESSAGE_RECEIVE if could not receive message from the
* kernel
* @return RMNETCTL_API_ERR_MESSAGE_TYPE if the request and response type do not
* match
*/
static uint16_t rmnetctl_transact(rmnetctl_hndl_t *hndl,
			struct rmnet_nl_msg_s *request,
			struct rmnet_nl_msg_s *response) {
	uint8_t *request_buf, *response_buf;
	struct nlmsghdr *nlmsghdr_val;
	struct rmnet_nl_msg_s *rmnet_nl_msg_s_val;
	ssize_t bytes_read = -1, buffsize = MAX_BUF_SIZE - sizeof(struct nlmsghdr);
	uint16_t return_code = RMNETCTL_API_ERR_HNDL_INVALID;
	struct sockaddr_nl* __attribute__((__may_alias__)) saddr_ptr;
	request_buf = NULL;
	response_buf = NULL;
	nlmsghdr_val = NULL;
	rmnet_nl_msg_s_val = NULL;
	do {
	if (!hndl){
		break;
	}
	if (!request){
		return_code = RMNETCTL_API_ERR_REQUEST_NULL;
		break;
	}
	if (!response){
		return_code = RMNETCTL_API_ERR_RESPONSE_NULL;
		break;
	}
	request_buf = (uint8_t *)malloc(MAX_BUF_SIZE * sizeof(uint8_t));
	if (!request_buf){
		return_code = RMNETCTL_API_ERR_REQUEST_NULL;
		break;
	}

	response_buf = (uint8_t *)malloc(MAX_BUF_SIZE * sizeof(uint8_t));
	if (!response_buf) {
		return_code = RMNETCTL_API_ERR_RESPONSE_NULL;
		break;
	}

	nlmsghdr_val = (struct nlmsghdr *)request_buf;
	rmnet_nl_msg_s_val = (struct rmnet_nl_msg_s *)NLMSG_DATA(request_buf);

	memset(request_buf, 0, MAX_BUF_SIZE*sizeof(uint8_t));
	memset(response_buf, 0, MAX_BUF_SIZE*sizeof(uint8_t));

	nlmsghdr_val->nlmsg_seq = hndl->transaction_id;
	nlmsghdr_val->nlmsg_pid = hndl->pid;
	nlmsghdr_val->nlmsg_len = MAX_BUF_SIZE;

	memscpy((void *)NLMSG_DATA(request_buf), buffsize, request,
	sizeof(struct rmnet_nl_msg_s));

	rmnet_nl_msg_s_val->crd = RMNET_NETLINK_MSG_COMMAND;
	hndl->transaction_id++;

	saddr_ptr = &hndl->dest_addr;
	socklen_t addrlen = sizeof(struct sockaddr_nl);
	if (sendto(hndl->netlink_fd,
			request_buf,
			MAX_BUF_SIZE,
			RMNETCTL_SOCK_FLAG,
			(struct sockaddr*)saddr_ptr,
			sizeof(struct sockaddr_nl)) < 0) {
		return_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		break;
	}

	saddr_ptr = &hndl->src_addr;
	bytes_read = recvfrom(hndl->netlink_fd,
			response_buf,
			MAX_BUF_SIZE,
			RMNETCTL_SOCK_FLAG,
			(struct sockaddr*)saddr_ptr,
			&addrlen);
	if (bytes_read < 0) {
		return_code = RMNETCTL_API_ERR_MESSAGE_RECEIVE;
		break;
	}
	buffsize = MAX_BUF_SIZE - sizeof(struct nlmsghdr);
	memscpy(response, buffsize, (void *)NLMSG_DATA(response_buf),
	sizeof(struct rmnet_nl_msg_s));
	if (sizeof(*response) < sizeof(struct rmnet_nl_msg_s)) {
		return_code = RMNETCTL_API_ERR_RESPONSE_NULL;
		break;
	}

	if (request->message_type != response->message_type) {
		return_code = RMNETCTL_API_ERR_MESSAGE_TYPE;
		break;
	}
	return_code = RMNETCTL_SUCCESS;
	} while(0);
	free(request_buf);
	free(response_buf);
	return return_code;
}

/*!
* @brief Static function to check the dev name
* @details Checks if the name is not NULL and if the name is less than the
* RMNET_MAX_STR_LEN
* @param dev_name Name of the device
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
static inline int _rmnetctl_check_dev_name(const char *dev_name) {
	int return_code = RMNETCTL_INVALID_ARG;
	do {
	if (!dev_name)
		break;
	if (strlen(dev_name) >= RMNET_MAX_STR_LEN)
		break;
	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

/*!
* @brief Static function to check the string length after a copy
* @details Checks if the string length is not lesser than zero and lesser than
* RMNET_MAX_STR_LEN
* @param str_len length of the string after a copy
* @param error_code Status code of this operation
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
*/
static inline int _rmnetctl_check_len(size_t str_len, uint16_t *error_code) {
	int return_code = RMNETCTL_LIB_ERR;
	do {
	if (str_len > RMNET_MAX_STR_LEN) {
		*error_code = RMNETCTL_API_ERR_STRING_TRUNCATION;
		break;
	}
	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

/*!
* @brief Static function to check the response type
* @details Checks if the response type of this message was return code
* @param crd The crd field passed
* @param error_code Status code of this operation
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
*/
static inline int _rmnetctl_check_code(int crd, uint16_t *error_code) {
	int return_code = RMNETCTL_LIB_ERR;
	do {
	if (crd != RMNET_NETLINK_MSG_RETURNCODE) {
		*error_code = RMNETCTL_API_ERR_RETURN_TYPE;
		break;
	}
	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

/*!
* @brief Static function to check the response type
* @details Checks if the response type of this message was data
* @param crd The crd field passed
* @param error_code Status code of this operation
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
*/
static inline int _rmnetctl_check_data(int crd, uint16_t *error_code) {
	int return_code = RMNETCTL_LIB_ERR;
	do {
	if (crd != RMNET_NETLINK_MSG_RETURNDATA) {
		*error_code = RMNETCTL_API_ERR_RETURN_TYPE;
		break;
	}
	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

/*!
* @brief Static function to set the return value
* @details Checks if the error_code from the transaction is zero for a return
* code type message and sets the message type as RMNETCTL_SUCCESS
* @param crd The crd field passed
* @param error_code Status code of this operation
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
*/
static inline int _rmnetctl_set_codes(int error_val, uint16_t *error_code) {
	int return_code = RMNETCTL_KERNEL_ERR;
	if (error_val == RMNET_CONFIG_OK)
		return_code = RMNETCTL_SUCCESS;
	else
		*error_code = (uint16_t)error_val + RMNETCTL_KERNEL_FIRST_ERR;
	return return_code;
}

/*===========================================================================
				EXPOSED API
===========================================================================*/

int rmnetctl_init(rmnetctl_hndl_t **hndl, uint16_t *error_code)
{
	pid_t pid = 0;
	int netlink_fd = -1, return_code = RMNETCTL_LIB_ERR;
	struct sockaddr_nl* __attribute__((__may_alias__)) saddr_ptr;
	do {
	if ((!hndl) || (!error_code)){
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	*hndl = (rmnetctl_hndl_t *)malloc(sizeof(rmnetctl_hndl_t));
	if (!*hndl) {
		*error_code = RMNETCTL_API_ERR_HNDL_INVALID;
		break;
	}

	memset(*hndl, 0, sizeof(rmnetctl_hndl_t));

	pid = getpid();
	if (pid  < MIN_VALID_PROCESS_ID) {
		free(*hndl);
		*error_code = RMNETCTL_INIT_ERR_PROCESS_ID;
		break;
	}
	(*hndl)->pid = (uint32_t)pid;
	netlink_fd = socket(PF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, RMNET_NETLINK_PROTO);
	if (netlink_fd < MIN_VALID_SOCKET_FD) {
		free(*hndl);
		*error_code = RMNETCTL_INIT_ERR_NETLINK_FD;
		break;
	}

	(*hndl)->netlink_fd = netlink_fd;

	memset(&(*hndl)->src_addr, 0, sizeof(struct sockaddr_nl));

	(*hndl)->src_addr.nl_family = AF_NETLINK;
	(*hndl)->src_addr.nl_pid = (*hndl)->pid;

	saddr_ptr = &(*hndl)->src_addr;
	if (bind((*hndl)->netlink_fd,
		(struct sockaddr*)saddr_ptr,
		sizeof(struct sockaddr_nl)) < 0) {
		close((*hndl)->netlink_fd);
		free(*hndl);
		*error_code = RMNETCTL_INIT_ERR_BIND;
		break;
	}

	memset(&(*hndl)->dest_addr, 0, sizeof(struct sockaddr_nl));

	(*hndl)->dest_addr.nl_family = AF_NETLINK;
	(*hndl)->dest_addr.nl_pid = KERNEL_PROCESS_ID;
	(*hndl)->dest_addr.nl_groups = UNICAST;

	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

void rmnetctl_cleanup(rmnetctl_hndl_t *hndl)
{
	if (!hndl)
		return;
	close(hndl->netlink_fd);
	free(hndl);
}

int rmnet_associate_network_device(rmnetctl_hndl_t *hndl,
				   const char *dev_name,
				   uint16_t *error_code,
				   uint8_t assoc_dev)
{
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!error_code) || _rmnetctl_check_dev_name(dev_name) ||
		((assoc_dev != RMNETCTL_DEVICE_ASSOCIATE) &&
		(assoc_dev != RMNETCTL_DEVICE_UNASSOCIATE))) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	if (assoc_dev == RMNETCTL_DEVICE_ASSOCIATE)
		request.message_type = RMNET_NETLINK_ASSOCIATE_NETWORK_DEVICE;
	else
		request.message_type = RMNET_NETLINK_UNASSOCIATE_NETWORK_DEVICE;

	request.arg_length = RMNET_MAX_STR_LEN;
	str_len = strlcpy((char *)(request.data), dev_name, (size_t)RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;
	if (_rmnetctl_check_code(response.crd, error_code) != RMNETCTL_SUCCESS)
		break;
	return_code = _rmnetctl_set_codes(response.return_code, error_code);
	} while(0);
	return return_code;
}

int rmnet_get_network_device_associated(rmnetctl_hndl_t *hndl,
					const char *dev_name,
					int *register_status,
					uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int  return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!register_status) || (!error_code) ||
	_rmnetctl_check_dev_name(dev_name)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	request.message_type = RMNET_NETLINK_GET_NETWORK_DEVICE_ASSOCIATED;

	request.arg_length = RMNET_MAX_STR_LEN;
	str_len = strlcpy((char *)(request.data), dev_name, RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;

	if (_rmnetctl_check_data(response.crd, error_code)
		!= RMNETCTL_SUCCESS) {
		if (_rmnetctl_check_code(response.crd, error_code)
			== RMNETCTL_SUCCESS)
			return_code = _rmnetctl_set_codes(response.return_code,
							  error_code);
		break;
	}

	*register_status = response.return_code;
	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

int rmnet_set_link_egress_data_format(rmnetctl_hndl_t *hndl,
				      uint32_t egress_flags,
				      uint16_t agg_size,
				      uint16_t agg_count,
				      const char *dev_name,
				      uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int  return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!error_code) || _rmnetctl_check_dev_name(dev_name) ||
	    ((~EGRESS_FLAGS_MASK) & egress_flags)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	request.message_type = RMNET_NETLINK_SET_LINK_EGRESS_DATA_FORMAT;

	request.arg_length = RMNET_MAX_STR_LEN +
			 sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
	str_len = strlcpy((char *)(request.data_format.dev),
			  dev_name,
			  RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	request.data_format.flags = egress_flags;
	request.data_format.agg_size = agg_size;
	request.data_format.agg_count = agg_count;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;

	if (_rmnetctl_check_code(response.crd, error_code) != RMNETCTL_SUCCESS)
		break;

	return_code = _rmnetctl_set_codes(response.return_code, error_code);
	} while(0);
	return return_code;
}

int rmnet_get_link_egress_data_format(rmnetctl_hndl_t *hndl,
				      const char *dev_name,
				      uint32_t *egress_flags,
				      uint16_t *agg_size,
				      uint16_t *agg_count,
				      uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int  return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!egress_flags) || (!agg_size) || (!agg_count) ||
	(!error_code) || _rmnetctl_check_dev_name(dev_name)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}
	request.message_type = RMNET_NETLINK_GET_LINK_EGRESS_DATA_FORMAT;

	request.arg_length = RMNET_MAX_STR_LEN;
	str_len = strlcpy((char *)(request.data_format.dev),
			  dev_name,
			  RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;

	if (_rmnetctl_check_data(response.crd, error_code)
		!= RMNETCTL_SUCCESS) {
		if (_rmnetctl_check_code(response.crd, error_code)
			== RMNETCTL_SUCCESS)
			return_code = _rmnetctl_set_codes(response.return_code,
							  error_code);
		break;
	}

	*egress_flags = response.data_format.flags;
	*agg_size = response.data_format.agg_size;
	*agg_count = response.data_format.agg_count;
	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

int rmnet_set_link_ingress_data_format_tailspace(rmnetctl_hndl_t *hndl,
						 uint32_t ingress_flags,
						 uint8_t  tail_spacing,
						 const char *dev_name,
						 uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int  return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!error_code) || _rmnetctl_check_dev_name(dev_name) ||
	    ((~INGRESS_FLAGS_MASK) & ingress_flags)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	request.message_type = RMNET_NETLINK_SET_LINK_INGRESS_DATA_FORMAT;

	request.arg_length = RMNET_MAX_STR_LEN +
	sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t);
	str_len = strlcpy((char *)(request.data_format.dev),
			  dev_name,
			  RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;
	request.data_format.flags = ingress_flags;
	request.data_format.tail_spacing = tail_spacing;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;

	if (_rmnetctl_check_code(response.crd, error_code) != RMNETCTL_SUCCESS)
		break;

	return_code = _rmnetctl_set_codes(response.return_code, error_code);
	} while(0);
	return return_code;
}

int rmnet_get_link_ingress_data_format_tailspace(rmnetctl_hndl_t *hndl,
						 const char *dev_name,
						 uint32_t *ingress_flags,
						 uint8_t  *tail_spacing,
						 uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int  return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!error_code) ||
		_rmnetctl_check_dev_name(dev_name)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	request.message_type = RMNET_NETLINK_GET_LINK_INGRESS_DATA_FORMAT;

	request.arg_length = RMNET_MAX_STR_LEN;
	str_len = strlcpy((char *)(request.data_format.dev),
			  dev_name,
			  RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;

	if (_rmnetctl_check_data(response.crd, error_code)
		!= RMNETCTL_SUCCESS) {
		if (_rmnetctl_check_code(response.crd, error_code)
			== RMNETCTL_SUCCESS)
			return_code = _rmnetctl_set_codes(response.return_code,
							  error_code);
		break;
	}

	if (ingress_flags)
		*ingress_flags = response.data_format.flags;

	if (tail_spacing)
		*tail_spacing = response.data_format.tail_spacing;

	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

int rmnet_set_logical_ep_config(rmnetctl_hndl_t *hndl,
				int32_t ep_id,
				uint8_t operating_mode,
				const char *dev_name,
				const char *next_dev,
				uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || ((ep_id < -1) || (ep_id > 31)) || (!error_code) ||
		_rmnetctl_check_dev_name(dev_name) ||
		_rmnetctl_check_dev_name(next_dev) ||
		operating_mode >= RMNET_EPMODE_LENGTH) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	request.message_type = RMNET_NETLINK_SET_LOGICAL_EP_CONFIG;

	request.arg_length = RMNET_MAX_STR_LEN +
	RMNET_MAX_STR_LEN + sizeof(int32_t) + sizeof(uint8_t);
	str_len = strlcpy((char *)(request.local_ep_config.dev),
			  dev_name,
			  RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	str_len = strlcpy((char *)(request.local_ep_config.next_dev),
			  next_dev,
			  RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;
	request.local_ep_config.ep_id = ep_id;
	request.local_ep_config.operating_mode = operating_mode;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;
	if (_rmnetctl_check_code(response.crd, error_code) != RMNETCTL_SUCCESS)
		break;

	return_code = _rmnetctl_set_codes(response.return_code, error_code);
	} while(0);
	return return_code;
}

int rmnet_unset_logical_ep_config(rmnetctl_hndl_t *hndl,
				  int32_t ep_id,
				  const char *dev_name,
				  uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int return_code = RMNETCTL_LIB_ERR;
	do {

	if ((!hndl) || ((ep_id < -1) || (ep_id > 31)) || (!error_code) ||
		_rmnetctl_check_dev_name(dev_name)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	request.message_type = RMNET_NETLINK_UNSET_LOGICAL_EP_CONFIG;

	request.arg_length = RMNET_MAX_STR_LEN + sizeof(int32_t);
	str_len = strlcpy((char *)(request.local_ep_config.dev),
			  dev_name,
			  RMNET_MAX_STR_LEN);

	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	request.local_ep_config.ep_id = ep_id;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;
	if (_rmnetctl_check_code(response.crd, error_code) != RMNETCTL_SUCCESS)
		break;

	return_code = _rmnetctl_set_codes(response.return_code, error_code);
	} while(0);

	return return_code;
}

int rmnet_get_logical_ep_config(rmnetctl_hndl_t *hndl,
				int32_t ep_id,
				const char *dev_name,
				uint8_t *operating_mode,
				char **next_dev,
				uint32_t next_dev_len,
				uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	size_t str_len = 0;
	int return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!operating_mode) || (!error_code) || ((ep_id < -1) ||
	    (ep_id > 31)) || _rmnetctl_check_dev_name(dev_name) || (!next_dev)
	    || (0 == next_dev_len)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	request.message_type = RMNET_NETLINK_GET_LOGICAL_EP_CONFIG;

	request.arg_length = RMNET_MAX_STR_LEN + sizeof(int32_t);
	str_len = strlcpy((char *)(request.local_ep_config.dev),
			  dev_name,
			  RMNET_MAX_STR_LEN);
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	request.local_ep_config.ep_id = ep_id;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;

	if (_rmnetctl_check_data(response.crd, error_code)
		!= RMNETCTL_SUCCESS) {
		if (_rmnetctl_check_code(response.crd, error_code)
			== RMNETCTL_SUCCESS)
			return_code = _rmnetctl_set_codes(response.return_code,
							  error_code);
		break;
	}

	str_len = strlcpy(*next_dev,
			  (char *)(response.local_ep_config.next_dev),
			  min(RMNET_MAX_STR_LEN, next_dev_len));
	if (_rmnetctl_check_len(str_len, error_code) != RMNETCTL_SUCCESS)
		break;

	*operating_mode = response.local_ep_config.operating_mode;
	return_code = RMNETCTL_SUCCESS;
	} while(0);
	return return_code;
}

int rmnet_new_vnd_prefix(rmnetctl_hndl_t *hndl,
			 uint32_t id,
			 uint16_t *error_code,
			 uint8_t new_vnd,
			 const char *prefix)
{
	struct rmnet_nl_msg_s request, response;
	int return_code = RMNETCTL_LIB_ERR;
	size_t str_len = 0;
	do {
	if ((!hndl) || (!error_code) ||
	((new_vnd != RMNETCTL_NEW_VND) && (new_vnd != RMNETCTL_FREE_VND))) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	memset(request.vnd.vnd_name, 0, RMNET_MAX_STR_LEN);
	if (new_vnd ==  RMNETCTL_NEW_VND) {
		if (prefix) {
			request.message_type =RMNET_NETLINK_NEW_VND_WITH_PREFIX;
			str_len = strlcpy((char *)request.vnd.vnd_name,
					  prefix, RMNET_MAX_STR_LEN);
			if (_rmnetctl_check_len(str_len, error_code)
						!= RMNETCTL_SUCCESS)
				break;
		} else {
			request.message_type = RMNET_NETLINK_NEW_VND;
		}
	} else {
		request.message_type = RMNET_NETLINK_FREE_VND;
	}

	request.arg_length = sizeof(uint32_t);
	request.vnd.id = id;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;
	if (_rmnetctl_check_code(response.crd, error_code) != RMNETCTL_SUCCESS)
		break;

	return_code = _rmnetctl_set_codes(response.return_code, error_code);
	} while(0);
	return return_code;
}

int rmnet_new_vnd_name(rmnetctl_hndl_t *hndl,
			 uint32_t id,
			 uint16_t *error_code,
			 const char *prefix)
{
	struct rmnet_nl_msg_s request, response;
	int return_code = RMNETCTL_LIB_ERR;
	size_t str_len = 0;
	do {
	if ((!hndl) || (!error_code)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	memset(request.vnd.vnd_name, 0, RMNET_MAX_STR_LEN);
		if (prefix) {
			request.message_type =RMNET_NETLINK_NEW_VND_WITH_NAME;
			str_len = strlcpy((char *)request.vnd.vnd_name,
					  prefix, RMNET_MAX_STR_LEN);
			if (_rmnetctl_check_len(str_len, error_code)
						!= RMNETCTL_SUCCESS)
				break;
		} else {
			request.message_type = RMNET_NETLINK_NEW_VND;
		}

	request.arg_length = sizeof(uint32_t);
	request.vnd.id = id;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;
	if (_rmnetctl_check_code(response.crd, error_code) != RMNETCTL_SUCCESS)
		break;

	return_code = _rmnetctl_set_codes(response.return_code, error_code);
	} while(0);
	return return_code;
}

int rmnet_new_vnd(rmnetctl_hndl_t *hndl,
		  uint32_t id,
		  uint16_t *error_code,
		  uint8_t new_vnd)
{
	return rmnet_new_vnd_prefix(hndl, id, error_code, new_vnd, 0);
}

int rmnet_get_vnd_name(rmnetctl_hndl_t *hndl,
		       uint32_t id,
		       uint16_t *error_code,
		       char *buf,
		       uint32_t buflen)
{
	struct rmnet_nl_msg_s request, response;
	uint32_t str_len;
	int return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!error_code) || (!buf) || (0 == buflen)) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}

	request.message_type = RMNET_NETLINK_GET_VND_NAME;
	request.arg_length = sizeof(uint32_t);
	request.vnd.id = id;


	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
		!= RMNETCTL_SUCCESS)
		break;

	if (_rmnetctl_check_data(response.crd, error_code)
		!= RMNETCTL_SUCCESS) {
		if (_rmnetctl_check_code(response.crd, error_code)
			== RMNETCTL_SUCCESS)
			return_code = _rmnetctl_set_codes(response.return_code,
							  error_code);
		break;
	}

	str_len = (uint32_t)strlcpy(buf,
			  (char *)(response.vnd.vnd_name),
			  buflen);
	if (str_len >= buflen) {
		*error_code = RMNETCTL_API_ERR_STRING_TRUNCATION;
		break;
	}

	return_code = RMNETCTL_SUCCESS;
	} while (0);
	return return_code;
}

int rmnet_add_del_vnd_tc_flow(rmnetctl_hndl_t *hndl,
			      uint32_t id,
			      uint32_t map_flow_id,
			      uint32_t tc_flow_id,
			      uint8_t set_flow,
			      uint16_t *error_code) {
	struct rmnet_nl_msg_s request, response;
	int return_code = RMNETCTL_LIB_ERR;
	do {
	if ((!hndl) || (!error_code) || ((set_flow != RMNETCTL_ADD_FLOW) &&
	    (set_flow != RMNETCTL_DEL_FLOW))) {
		return_code = RMNETCTL_INVALID_ARG;
		break;
	}
	if (set_flow ==  RMNETCTL_ADD_FLOW)
		request.message_type = RMNET_NETLINK_ADD_VND_TC_FLOW;
	else
		request.message_type = RMNET_NETLINK_DEL_VND_TC_FLOW;

	request.arg_length = (sizeof(uint32_t))*3;
	request.flow_control.id = id;
	request.flow_control.map_flow_id = map_flow_id;
	request.flow_control.tc_flow_id = tc_flow_id;

	if ((*error_code = rmnetctl_transact(hndl, &request, &response))
	!= RMNETCTL_SUCCESS)
		break;
	if (_rmnetctl_check_code(response.crd, error_code) != RMNETCTL_SUCCESS)
		break;
	return_code = _rmnetctl_set_codes(response.return_code, error_code);
	} while(0);
	return return_code;
}

/*
 *                       NEW DRIVER API
 */

 /* @brief Add a Routing Attribute to a Netlink message
  * @param *req The Netlink message we're adding to
  * @param *reqsize The remaining space within the Netlink message
  * @param type The type of the RTA to add
  * @param len The length of the RTA data to add
  * @param *data A pointer to the RTA data to add
  * @return RMNETCTL_LIB_ERR if there is not enough space to add the RTA
  * @return RMNETCTL_SUCCESS if we added the RTA successfully
  */
static int rta_put(struct nlmsg *req, size_t *reqsize, int type, int len,
		   void *data)
{
	struct rtattr *attr = NLMSG_TAIL(&req->nl_addr);

	attr->rta_type = type;
	attr->rta_len = RTA_LENGTH(len);
	CHECK_MEMSCPY(memscpy_repeat(RTA_DATA(attr), reqsize, data, len));
	req->nl_addr.nlmsg_len = NLMSG_ALIGN(req->nl_addr.nlmsg_len) +
				 RTA_ALIGN(attr->rta_len);

	return RMNETCTL_SUCCESS;
}

static int rta_put_u8(struct nlmsg *req, size_t *reqsize, int type,
		       uint8_t data)
{
	return rta_put(req, reqsize, type, sizeof(data), &data);
}

/* @brief Add an RTA to the Netlink message with a u16 data
 * @param *req The Netlink message
 * @param *reqsize The remainins space within the Netlink message
 * @param type The type of the RTA to add
 * @param data The data of the RTA
 * @rteturn RMNETCTL_LIB_ERR if there is not enough space to add the RTA
 * @return RMNETCTL_SUCCESS if we added the RTA successfully
 */
static int rta_put_u16(struct nlmsg *req, size_t *reqsize, int type,
		       uint16_t data)
{
	return rta_put(req, reqsize, type, sizeof(data), &data);
}

/* @brief Add an RTA to the Netlink message with a u32 data
 * @param *req The Netlink message
 * @param *reqsize The remainins space within the Netlink message
 * @param type The type of the RTA to add
 * @param data The data of the RTA
 * @rteturn RMNETCTL_LIB_ERR if there is not enough space to add the RTA
 * @return RMNETCTL_SUCCESS if we added the RTA successfully
 */
static int rta_put_u32(struct nlmsg *req, size_t *reqsize, int type,
		       uint32_t data)
{
	return rta_put(req, reqsize, type, sizeof(data), &data);
}

/* @brief Add an RTA to the Netlink message with string data
 * @param *req The Netlink message
 * @param *reqsize The remainins space within the Netlink message
 * @param type The type of the RTA to add
 * @param *data The data of the RTA
 * @rteturn RMNETCTL_LIB_ERR if there is not enough space to add the RTA
 * @return RMNETCTL_SUCCESS if we added the RTA successfully
 */
static int rta_put_string(struct nlmsg *req, size_t *reqsize, int type,
			  char *data)
{
	return rta_put(req, reqsize, type, strlen(data) + 1, data);
}

/* @brief Start a nested RTA within the Netlink message
 * @param *req The Netlink message
 * @param *reqsize The remainins space within the Netlink message
 * @param type The type of the RTA to add
 * @param **start A pointer where we store the start of the ensted attribute
 * @rteturn RMNETCTL_LIB_ERR if there is not enough space to add the RTA
 * @return RMNETCTL_SUCCESS if we added the RTA successfully
 */
static int rta_nested_start(struct nlmsg *req, size_t *reqsize, int type,
			    struct rtattr **start)
{
	*start = NLMSG_TAIL(&req->nl_addr);
	return rta_put(req, reqsize, type, 0, NULL);
}

/* @brief End a nested RTA previously started with rta_nested_start
 * @param *req The Netlink message
 * @param *start The start of the nested RTA, as provided by rta_nested_start
 */
static void rta_nested_end(struct nlmsg *req, struct rtattr *start)
{
	start->rta_len = (char *)NLMSG_TAIL(&req->nl_addr) - (char *)start;
}

static void rta_parse(struct rtattr **tb, int maxtype, struct rtattr *head,
		      int len)
{
	struct rtattr *rta;

	memset(tb, 0, sizeof(struct rtattr *) * maxtype);
	for (rta = head; RTA_OK(rta, len);
	     rta = RTA_NEXT(rta, len)) {
		__u16 type = rta->rta_type & NLA_TYPE_MASK;

		if (type > 0 && type < maxtype)
			tb[type] = rta;
	}
}

static struct rtattr *rta_find(struct rtattr *rta, int attrlen, uint16_t type)
{
	for (; RTA_OK(rta, attrlen); rta = RTA_NEXT(rta, attrlen)) {
		if (rta->rta_type == (type & NLA_TYPE_MASK))
			return rta;
	}

	return NULL;
}

/* @brief Fill a Netlink messages with the necessary common RTAs for creating a
 * RTM_NEWLINK message for creating or changing rmnet devices.
 * @param *req The netlink message
 * @param *reqsize The remaining space within the Netlink message
 * @param devindex The ifindex of the physical device
 * @param *vndname The name of the rmnet device
 * @param index The MUX ID of the rmnet device
 * @param flagconfig The dataformat flags for the rmnet device
 * @return RMNETCTL_LIB_ERR if there is not enough space to add all RTAs
 * @return RMNETCTL_SUCCESS if everything was added successfully
 */
static int rmnet_fill_newlink_msg(struct nlmsg *req, size_t *reqsize,
				  unsigned int devindex, char *vndname,
				  uint8_t index, uint32_t flagconfig,
				  uint8_t offload)
{
	struct rtattr *linkinfo, *datainfo;
	struct ifla_vlan_flags flags;
	int rc;

	/* Set up link attr with devindex as data */
	rc = rta_put_u32(req, reqsize, IFLA_LINK, devindex);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rta_put_string(req, reqsize, IFLA_IFNAME, vndname);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	/* Set up info kind RMNET that has linkinfo and type */
	rc = rta_nested_start(req, reqsize, IFLA_LINKINFO, &linkinfo);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rta_put_string(req, reqsize, IFLA_INFO_KIND, "rmnet");
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rta_nested_start(req, reqsize, IFLA_INFO_DATA, &datainfo);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rta_put_u16(req, reqsize, RMNETCTL_IFLA_MUX_ID, index);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	if (flagconfig != 0) {
		flags.mask = flagconfig;
		flags.flags = flagconfig;

		rc = rta_put(req, reqsize, RMNETCTL_IFLA_FLAGS, sizeof(flags),
			     &flags);
		if (rc != RMNETCTL_SUCCESS)
			return rc;
	}

	rc = rta_put_u8(req, reqsize, RMNETCTL_IFLA_NSS_OFFLOAD, offload);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rta_nested_end(req, datainfo);
	rta_nested_end(req, linkinfo);

	return RMNETCTL_SUCCESS;
}

/* @brief Add all necessary RTA elements to a Netlink message suitable for
 * sending to the DFC driver
 * @param *req The Netlink message
 * @param *reqsize The remaining space in the Netlink message
 * @param devindex The ifindex of the real physical device
 * @param *vndname The name of the VND we're modifying
 * @param *flowinfo The parameters sent to the DFC driver
 * @return RMENTCTL_LIB_ERR if there is not enough space to add the RTAs
 * @return RMNETCTL_SUCCESS if everything was added successfully
 */
static int rmnet_fill_flow_msg(struct nlmsg *req, size_t *reqsize,
			       unsigned int devindex, char *vndname,
			       char *flowinfo, size_t flowlen)
{
	struct rtattr *linkinfo, *datainfo;
	int rc;

	/* Set up link attr with devindex as data */
	rc = rta_put_u32(req, reqsize, IFLA_LINK, devindex);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rta_put_string(req, reqsize, IFLA_IFNAME, vndname);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	/* Set up IFLA info kind RMNET that has linkinfo and type */
	rc = rta_nested_start(req, reqsize, IFLA_LINKINFO, &linkinfo);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rta_put_string(req, reqsize, IFLA_INFO_KIND, "rmnet");
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rta_nested_start(req, reqsize, IFLA_INFO_DATA, &datainfo);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rta_put(req, reqsize, RMNETCTL_IFLA_DFC_QOS, flowlen,
		     flowinfo);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rta_nested_end(req, datainfo);
	rta_nested_end(req, linkinfo);

	return RMNETCTL_SUCCESS;
}

/* @brief Synchronous method to receive messages to and from the kernel
 * using netlink sockets
 * @details Receives the ack response from the kernel.
 * @param *hndl RmNet handle for this transaction
 * @param *error_code Error code if transaction fails
 * @return RMNETCTL_API_SUCCESS if successfully able to send and receive message
 * from the kernel
 * @return RMNETCTL_API_ERR_HNDL_INVALID if RmNet handle for the transaction was
 * NULL
 * @return RMNETCTL_API_ERR_MESSAGE_RECEIVE if could not receive message from
 * the kernel
 * @return RMNETCTL_API_ERR_MESSAGE_TYPE if the response type does not
 * match
 */
static int rmnet_get_ack(rmnetctl_hndl_t *hndl, uint16_t *error_code)
{
	struct nlack {
		struct nlmsghdr ackheader;
		struct nlmsgerr ackdata;
		char   data[256];

	} ack;
	int i;

	if (!hndl || !error_code)
		return RMNETCTL_INVALID_ARG;

	if ((i = recv(hndl->netlink_fd, &ack, sizeof(ack), 0)) < 0) {
		*error_code = errno;
		return RMNETCTL_API_ERR_MESSAGE_RECEIVE;
	}

	/*Ack should always be NLMSG_ERROR type*/
	if (ack.ackheader.nlmsg_type == NLMSG_ERROR) {
		if (ack.ackdata.error == 0) {
			*error_code = RMNETCTL_API_SUCCESS;
			return RMNETCTL_SUCCESS;
		} else {
			*error_code = -ack.ackdata.error;
			return RMNETCTL_KERNEL_ERR;
		}
	}

	*error_code = RMNETCTL_API_ERR_RETURN_TYPE;
	return RMNETCTL_API_FIRST_ERR;
}

/*
 *                       EXPOSED NEW DRIVER API
 */
int rtrmnet_ctl_init(rmnetctl_hndl_t **hndl, uint16_t *error_code)
{
	struct sockaddr_nl __attribute__((__may_alias__)) *saddr_ptr;
	int netlink_fd = -1;
	pid_t pid = 0;

	if (!hndl || !error_code)
		return RMNETCTL_INVALID_ARG;

	*hndl = (rmnetctl_hndl_t *)malloc(sizeof(rmnetctl_hndl_t));
	if (!*hndl) {
		*error_code = RMNETCTL_API_ERR_HNDL_INVALID;
		return RMNETCTL_LIB_ERR;
	}

	memset(*hndl, 0, sizeof(rmnetctl_hndl_t));

	pid = getpid();
	if (pid  < MIN_VALID_PROCESS_ID) {
		free(*hndl);
		*error_code = RMNETCTL_INIT_ERR_PROCESS_ID;
		return RMNETCTL_LIB_ERR;
	}
	(*hndl)->pid = KERNEL_PROCESS_ID;
	netlink_fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_ROUTE);
	if (netlink_fd < MIN_VALID_SOCKET_FD) {
		free(*hndl);
		*error_code = RMNETCTL_INIT_ERR_NETLINK_FD;
		return RMNETCTL_LIB_ERR;
	}

	(*hndl)->netlink_fd = netlink_fd;

	memset(&(*hndl)->src_addr, 0, sizeof(struct sockaddr_nl));

	(*hndl)->src_addr.nl_family = AF_NETLINK;
	(*hndl)->src_addr.nl_pid = (*hndl)->pid;

	saddr_ptr = &(*hndl)->src_addr;
	if (bind((*hndl)->netlink_fd,
		(struct sockaddr *)saddr_ptr,
		sizeof(struct sockaddr_nl)) < 0) {
		close((*hndl)->netlink_fd);
		free(*hndl);
		*error_code = RMNETCTL_INIT_ERR_BIND;
		return RMNETCTL_LIB_ERR;
	}

	memset(&(*hndl)->dest_addr, 0, sizeof(struct sockaddr_nl));

	(*hndl)->dest_addr.nl_family = AF_NETLINK;
	(*hndl)->dest_addr.nl_pid = KERNEL_PROCESS_ID;
	(*hndl)->dest_addr.nl_groups = UNICAST;

	return RMNETCTL_SUCCESS;
}

int rtrmnet_ctl_deinit(rmnetctl_hndl_t *hndl)
{
	if (!hndl)
		return RMNETCTL_SUCCESS;

	close(hndl->netlink_fd);
	free(hndl);

	return RMNETCTL_SUCCESS;
}

int rtrmnet_ctl_newvnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
		       uint16_t *error_code, uint8_t  index,
		       uint32_t flagconfig, uint8_t offload)
{
	unsigned int devindex = 0;
	struct nlmsg req;
	size_t reqsize;
	int rc;

	if (!hndl || !devname || !vndname || !error_code ||
	   _rmnetctl_check_dev_name(vndname) || _rmnetctl_check_dev_name(devname))
		return RMNETCTL_INVALID_ARG;

	memset(&req, 0, sizeof(req));
	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_EXCL |
				  NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
	rc = rta_put_u32(&req, &reqsize, RMNET_IFLA_NUM_TX_QUEUES,
			 RMNETCTL_NUM_TX_QUEUES);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	rc = rmnet_fill_newlink_msg(&req, &reqsize, devindex, vndname, index,
				    flagconfig, offload);
	if (rc != RMNETCTL_SUCCESS)
		return rc;

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}

int rtrmnet_ctl_delvnd(rmnetctl_hndl_t *hndl, char *vndname,
		       uint16_t *error_code)
{
	unsigned int devindex = 0;
	struct nlmsg req;

	if (!hndl || !vndname || !error_code)
		return RMNETCTL_INVALID_ARG;

	memset(&req, 0, sizeof(req));
	req.nl_addr.nlmsg_type = RTM_DELLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of vndname*/
	devindex = if_nametoindex(vndname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	/* Setup index attribute */
	req.ifmsg.ifi_index = devindex;
	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}


int rtrmnet_ctl_changevnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
			  uint16_t *error_code, uint8_t  index,
			  uint32_t flagconfig, uint8_t offload)
{
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));

	if (!hndl || !devname || !vndname || !error_code ||
	    _rmnetctl_check_dev_name(vndname) || _rmnetctl_check_dev_name(devname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	rc = rmnet_fill_newlink_msg(&req, &reqsize, devindex, vndname, index,
				    flagconfig, offload);
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}

int rtrmnet_ctl_getvnd(rmnetctl_hndl_t *hndl, char *vndname,
		       uint16_t *error_code, uint16_t *mux_id,
		       uint32_t *flagconfig, uint8_t *agg_count,
		       uint16_t *agg_size, uint32_t *agg_time,
		       uint8_t *features, uint8_t *offload)
{
	struct nlmsg req;
	struct nlmsghdr *resp;
	struct rtattr *attrs, *linkinfo, *datainfo;
	struct rtattr *tb[__RMNETCTL_IFLA_MAX];
	unsigned int devindex = 0;
	int resp_len;

	memset(&req, 0, sizeof(req));

	if (!hndl || !vndname || !error_code || !(mux_id || flagconfig) ||
	    _rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	req.nl_addr.nlmsg_type = RTM_GETLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of vndname */
	devindex = if_nametoindex(vndname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	req.ifmsg.ifi_index = devindex;
	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	resp_len = recv(hndl->netlink_fd, NULL, 0, MSG_PEEK | MSG_TRUNC);
	if (resp_len < 0) {
		*error_code = errno;
		return RMNETCTL_API_ERR_MESSAGE_RECEIVE;
	}

	resp = malloc((size_t)resp_len);
	if (!resp) {
		*error_code = errno;
		return RMNETCTL_LIB_ERR;
	}

	resp_len = recv(hndl->netlink_fd, (char *)resp, (size_t)resp_len, 0);
	if (resp_len < 0) {
		*error_code = errno;
		free(resp);
		return RMNETCTL_API_ERR_MESSAGE_RECEIVE;
	}

	/* Parse out the RT attributes */
	attrs = (struct rtattr *)((char *)NLMSG_DATA(resp) +
				  NLMSG_ALIGN(sizeof(req.ifmsg)));
	linkinfo = rta_find(attrs, NLMSG_PAYLOAD(resp, sizeof(req.ifmsg)),
			    IFLA_LINKINFO);
	if (!linkinfo) {
		free(resp);
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return RMNETCTL_KERNEL_ERR;
	}

	datainfo = rta_find(RTA_DATA(linkinfo), RTA_PAYLOAD(linkinfo),
			    IFLA_INFO_DATA);
	if (!datainfo) {
		free(resp);
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return RMNETCTL_KERNEL_ERR;
	}

	/* Parse all the rmnet-specific information from the kernel */
	rta_parse(tb, __RMNETCTL_IFLA_MAX, RTA_DATA(datainfo),
		  RTA_PAYLOAD(datainfo));
	if (tb[RMNETCTL_IFLA_MUX_ID] && mux_id)
		*mux_id = *((uint16_t *)RTA_DATA(tb[RMNETCTL_IFLA_MUX_ID]));
	if (tb[RMNETCTL_IFLA_FLAGS] && flagconfig) {
		struct ifla_vlan_flags *flags;

		flags = (struct ifla_vlan_flags *)
			 RTA_DATA(tb[RMNETCTL_IFLA_FLAGS]);
		*flagconfig = flags->flags;
	}
	if (tb[RMNETCTL_IFLA_UPLINK_PARAMS]) {
		struct rmnetctl_uplink_params *ul_agg;

		ul_agg = (struct rmnetctl_uplink_params *)
			 RTA_DATA(tb[RMNETCTL_IFLA_UPLINK_PARAMS]);

		if (agg_size)
			*agg_size = ul_agg->byte_count;

		if (agg_count)
			*agg_count = ul_agg->packet_count;

		if (features)
			*features = ul_agg->features;

		if (agg_time)
			*agg_time = ul_agg->time_limit;
	}
	if (tb[RMNETCTL_IFLA_NSS_OFFLOAD] && offload)
		*offload = *((uint8_t *)RTA_DATA(tb[RMNETCTL_IFLA_NSS_OFFLOAD]));

	free(resp);
	return RMNETCTL_API_SUCCESS;
}

int rtrmnet_ctl_bridgevnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
			  uint16_t *error_code)
{
	unsigned int devindex = 0, vndindex = 0;
	struct nlmsg req;
	size_t reqsize;
	int rc;

	if (!hndl || !vndname || !devname || !error_code || _rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	memset(&req, 0, sizeof(req));
	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of vndname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	vndindex = if_nametoindex(vndname);
	if (vndindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	/* Setup index attribute */
	req.ifmsg.ifi_index = devindex;
	rc = rta_put_u32(&req, &reqsize, IFLA_MASTER, vndindex);
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}

int rtrmnet_set_uplink_aggregation_params(rmnetctl_hndl_t *hndl,
					  char *devname,
					  char *vndname,
					  uint8_t packet_count,
					  uint16_t byte_count,
					  uint32_t time_limit,
					  uint8_t features,
					  uint16_t *error_code)
{
	struct nlmsg req;
	struct rmnetctl_uplink_params uplink_params;
	struct rtattr *linkinfo, *datainfo;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&uplink_params, 0, sizeof(uplink_params));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	/* Set up link attr with devindex as data */
	rc = rta_put_u32(&req, &reqsize, IFLA_LINK, devindex);
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	rc = rta_put_string(&req, &reqsize, IFLA_IFNAME, vndname);
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	/* Set up IFLA info kind RMNET that has linkinfo and type */
	rc = rta_nested_start(&req, &reqsize, IFLA_LINKINFO, &linkinfo);
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	rc = rta_put_string(&req, &reqsize, IFLA_INFO_KIND, "rmnet");
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	rc = rta_nested_start(&req, &reqsize, IFLA_INFO_DATA, &datainfo);
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	uplink_params.byte_count = byte_count;
	uplink_params.packet_count = packet_count;
	uplink_params.features = features;
	uplink_params.time_limit = time_limit;
	rc = rta_put(&req, &reqsize, RMNETCTL_IFLA_UPLINK_PARAMS,
		     sizeof(uplink_params), &uplink_params);
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	rta_nested_end(&req, datainfo);
	rta_nested_end(&req, linkinfo);

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);

}

int rtrmnet_activate_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint32_t flow_id,
			  int ip_type,
			  uint32_t tcm_handle,
			  uint16_t *error_code)
{
	struct tcmsg  flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize =0;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));


	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm_handle = tcm_handle;
	flowinfo.tcm_family = RMNET_FLOW_MSG_ACTIVATE;
	flowinfo.tcm__pad1 = bearer_id;
	flowinfo.tcm_ifindex = ip_type;
	flowinfo.tcm_parent = flow_id;

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}


int rtrmnet_delete_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint32_t flow_id,
			  int ip_type,
			  uint16_t *error_code)
{
	struct tcmsg  flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm_family = RMNET_FLOW_MSG_DEACTIVATE;
	flowinfo.tcm_ifindex = ip_type;
	flowinfo.tcm__pad1 = bearer_id;
	flowinfo.tcm_parent = flow_id;

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}

int rtrmnet_control_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint16_t sequence,
			  uint32_t grantsize,
			  uint8_t ack,
			  uint16_t *error_code)
{
	struct tcmsg  flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm_family = RMNET_FLOW_MSG_CONTROL;
	flowinfo.tcm__pad1 = bearer_id;
	flowinfo.tcm__pad2 = sequence;
	flowinfo.tcm_parent = ack;
	flowinfo.tcm_info = grantsize;

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}


int rtrmnet_flow_state_up(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t instance,
			  uint32_t ep_type,
			  uint32_t ifaceid,
			  int flags,
			  uint16_t *error_code)
{
	struct tcmsg  flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm_handle = instance;
	flowinfo.tcm_family = RMNET_FLOW_MSG_UP;
	flowinfo.tcm_ifindex = flags;
	flowinfo.tcm_parent = ifaceid;
	flowinfo.tcm_info = ep_type;

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}


int rtrmnet_flow_state_down(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t instance,
			  uint16_t *error_code)
{
	struct tcmsg  flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm_handle = instance;
	flowinfo.tcm_family = RMNET_FLOW_MSG_DOWN;

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}

int rtrmnet_set_qmi_scale(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t scale,
			  uint16_t *error_code)
{
	struct tcmsg  flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname) || !scale)
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm_ifindex = scale;
	flowinfo.tcm_family = RMNET_FLOW_MSG_QMI_SCALE;

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}

int rtrmnet_set_wda_freq(rmnetctl_hndl_t *hndl,
			 char *devname,
			 char *vndname,
			 uint32_t freq,
			 uint16_t *error_code)
{
	struct tcmsg  flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm_ifindex = freq;
	flowinfo.tcm_family = RMNET_FLOW_MSG_WDA_FREQ;

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}

int rtrmnet_add_filter(rmnetctl_hndl_t *hndl,
		       char *devname,
		       char *vndname,
		       uint32_t flow_id,
		       int ip_type,
		       struct rmnetctl_filter *filter,
		       uint16_t *error_code)
{
	struct {
		struct tcmsg tcm;
		struct rmnetctl_filter filter;
	} flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname) || !filter)
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm.tcm_family = RMNET_FLOW_MSG_ADD_FILTER;
	flowinfo.tcm.tcm_ifindex = ip_type;
	flowinfo.tcm.tcm_parent = flow_id;
	memcpy(&flowinfo.filter, filter, sizeof(struct rmnetctl_filter));

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}

int rtrmnet_remove_filters(rmnetctl_hndl_t *hndl,
			   char *devname,
			   char *vndname,
			   uint32_t flow_id,
			   int ip_type,
			   uint16_t *error_code)
{
	struct tcmsg flowinfo;
	struct nlmsg req;
	unsigned int devindex = 0;
	size_t reqsize;
	int rc;

	memset(&req, 0, sizeof(req));
	memset(&flowinfo, 0, sizeof(flowinfo));

	if (!hndl || !devname || !error_code ||_rmnetctl_check_dev_name(devname) ||
		_rmnetctl_check_dev_name(vndname))
		return RMNETCTL_INVALID_ARG;

	reqsize = NLMSG_DATA_SIZE - sizeof(struct rtattr);
	req.nl_addr.nlmsg_type = RTM_NEWLINK;
	req.nl_addr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nl_addr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	req.nl_addr.nlmsg_seq = hndl->transaction_id;
	hndl->transaction_id++;

	/* Get index of devname*/
	devindex = if_nametoindex(devname);
	if (devindex == 0) {
		*error_code = errno;
		return RMNETCTL_KERNEL_ERR;
	}

	flowinfo.tcm_family = RMNET_FLOW_MSG_REMOVE_FILTERS;
	flowinfo.tcm_ifindex = ip_type;
	flowinfo.tcm_parent = flow_id;

	rc = rmnet_fill_flow_msg(&req, &reqsize, devindex, vndname,
				 (char *)&flowinfo, sizeof(flowinfo));
	if (rc != RMNETCTL_SUCCESS) {
		*error_code = RMNETCTL_API_ERR_RTA_FAILURE;
		return rc;
	}

	if (send(hndl->netlink_fd, &req, req.nl_addr.nlmsg_len, 0) < 0) {
		*error_code = RMNETCTL_API_ERR_MESSAGE_SEND;
		return RMNETCTL_LIB_ERR;
	}

	return rmnet_get_ack(hndl, error_code);
}
