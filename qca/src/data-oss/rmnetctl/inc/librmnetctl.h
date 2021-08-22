/******************************************************************************

			  L I B R M N E T C T L . H

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
*  @file    librmnetctl.h
*  @brief   rmnet control API's header file
*/

#ifndef LIBRMNETCTL_H
#define LIBRMNETCTL_H

/* RMNET API succeeded */
#define RMNETCTL_SUCCESS 0
/* RMNET API encountered an error while executing within the library. Check the
* error code in this case */
#define RMNETCTL_LIB_ERR 1
/* RMNET API encountered an error while executing in the kernel. Check the
* error code in this case */
#define RMNETCTL_KERNEL_ERR 2
/* RMNET API encountered an error because of invalid arguments*/
#define RMNETCTL_INVALID_ARG 3

/* Flag to associate a network device*/
#define RMNETCTL_DEVICE_ASSOCIATE 1
/* Flag to unassociate a network device*/
#define RMNETCTL_DEVICE_UNASSOCIATE 0
/* Flag to create a new virtual network device*/
#define RMNETCTL_NEW_VND 1
/* Flag to free a new virtual network device*/
#define RMNETCTL_FREE_VND 0
/* Flag to add a new flow*/
#define RMNETCTL_ADD_FLOW 1
/* Flag to delete an existing flow*/
#define RMNETCTL_DEL_FLOW 0

enum rmnetctl_error_codes_e {
	/* API succeeded. This should always be the first element. */
	RMNETCTL_API_SUCCESS = 0,

	RMNETCTL_API_FIRST_ERR = 1,
	/* API failed because not enough memory to create buffer to send
	 * message */
	RMNETCTL_API_ERR_REQUEST_INVALID = RMNETCTL_API_FIRST_ERR,
	/* API failed because not enough memory to create buffer for the
	 *  response message */
	RMNETCTL_API_ERR_RESPONSE_INVALID = 2,
	/* API failed because could not send the message to kernel */
	RMNETCTL_API_ERR_MESSAGE_SEND = 3,
	/* API failed because could not receive message from the kernel */
	RMNETCTL_API_ERR_MESSAGE_RECEIVE = 4,

	RMNETCTL_INIT_FIRST_ERR = 5,
	/* Invalid process id. So return an error. */
	RMNETCTL_INIT_ERR_PROCESS_ID = RMNETCTL_INIT_FIRST_ERR,
	/* Invalid socket descriptor id. So return an error. */
	RMNETCTL_INIT_ERR_NETLINK_FD = 6,
	/* Could not bind the socket to the Netlink file descriptor */
	RMNETCTL_INIT_ERR_BIND = 7,
	/* Invalid user id. Only root has access to this function. (NA) */
	RMNETCTL_INIT_ERR_INVALID_USER = 8,

	RMNETCTL_API_SECOND_ERR = 9,
	/* API failed because the RmNet handle for the transaction was NULL */
	RMNETCTL_API_ERR_HNDL_INVALID = RMNETCTL_API_SECOND_ERR,
	/* API failed because the request buffer for the transaction was NULL */
	RMNETCTL_API_ERR_REQUEST_NULL = 10,
	/* API failed because the response buffer for the transaction was NULL*/
	RMNETCTL_API_ERR_RESPONSE_NULL = 11,
	/* API failed because the request and response type do not match*/
	RMNETCTL_API_ERR_MESSAGE_TYPE = 12,
	/* API failed because the return type is invalid */
	RMNETCTL_API_ERR_RETURN_TYPE = 13,
	/* API failed because the string was truncated */
	RMNETCTL_API_ERR_STRING_TRUNCATION = 14,

	/* These error are 1-to-1 with rmnet_data config errors in rmnet_data.h
	   for each conversion.
	   please keep the enums synced.
	*/
	RMNETCTL_KERNEL_FIRST_ERR = 15,
	/* No error */
	RMNETCTL_KERNEL_ERROR_NO_ERR = RMNETCTL_KERNEL_FIRST_ERR,
	/* Invalid / unsupported message */
	RMNETCTL_KERNEL_ERR_UNKNOWN_MESSAGE = 16,
	/* Internal problem in the kernel module */
	RMNETCTL_KERNEL_ERR_INTERNAL = 17,
	/* Kernel is temporarily out of memory */
	RMNETCTL_KERNEL_ERR_OUT_OF_MEM = 18,
	/* Device already exists / Still in use */
	RMETNCTL_KERNEL_ERR_DEVICE_IN_USE = 19,
	/* Invalid request / Unsupported scenario */
	RMNETCTL_KERNEL_ERR_INVALID_REQUEST = 20,
	/* Device doesn't exist */
	RMNETCTL_KERNEL_ERR_NO_SUCH_DEVICE = 21,
	/* One or more of the arguments is invalid */
	RMNETCTL_KERNEL_ERR_BAD_ARGS = 22,
	/* Egress device is invalid */
	RMNETCTL_KERNEL_ERR_BAD_EGRESS_DEVICE = 23,
	/* TC handle is full */
	RMNETCTL_KERNEL_ERR_TC_HANDLE_FULL = 24,

	RMNETCTL_API_THIRD_ERR = 25,
	/* Failed to copy data into netlink message */
	RMNETCTL_API_ERR_RTA_FAILURE = RMNETCTL_API_THIRD_ERR,

	/* This should always be the last element */
	RMNETCTL_API_ERR_ENUM_LENGTH
};

#define RMNETCTL_ERR_MSG_SIZE 100

/*!
* @brief Contains a list of error message from API
*/
#ifndef RMNETCTL_NO_ERR_CODE
char rmnetctl_error_code_text
[RMNETCTL_API_ERR_ENUM_LENGTH][RMNETCTL_ERR_MSG_SIZE] = {
	"ERROR: API succeeded\n",
	"ERROR: Unable to allocate the buffer to send message\n",
	"ERROR: Unable to allocate the buffer to receive message\n",
	"ERROR: Could not send the message to kernel\n",
	"ERROR: Unable to receive message from the kernel\n",
	"ERROR: Invalid process id\n",
	"ERROR: Invalid socket descriptor id\n",
	"ERROR: Could not bind to netlink socket\n",
	"ERROR: Only root can access this API\n",
	"ERROR: RmNet handle for the transaction was NULL\n",
	"ERROR: Request buffer for the transaction was NULL\n",
	"ERROR: Response buffer for the transaction was NULL\n",
	"ERROR: Request and response type do not match\n",
	"ERROR: Return type is invalid\n",
	"ERROR: String was truncated\n",
	/* Kernel errors */
	"ERROR: Kernel call succeeded\n",
	"ERROR: Invalid / Unsupported directive\n",
	"ERROR: Internal problem in the kernel module\n",
	"ERROR: The kernel is temporarily out of memory\n",
	"ERROR: Device already exists / Still in use\n",
	"ERROR: Invalid request / Unsupported scenario\n",
	"ERROR: Device doesn't exist\n",
	"ERROR: One or more of the arguments is invalid\n",
	"ERROR: Egress device is invalid\n",
	"ERROR: TC handle is full\n",
	/* New Rmnet Driver Errors */
	"ERROR: Netlink message is too small to hold all data\n",
};
#endif /* RMNETCTL_NO_ERR_CODE */

#define RMNETCTL_FILTER_MASK_SADDR 0x1
#define RMNETCTL_FILTER_MASK_DADDR 0x2
#define RMNETCTL_FILTER_MASK_SPORT 0x4
#define RMNETCTL_FILTER_MASK_DPORT 0x8
#define RMNETCTL_FILTER_MASK_PROTO 0x10
#define RMNETCTL_FILTER_MASK_TOS   0x20

struct rmnetctl_filter {
	uint32_t saddr[4];
	uint32_t smask[4];
	uint32_t daddr[4];
	uint32_t dmask[4];
	uint16_t sport;
	uint16_t sport_range;
	uint16_t dport;
	uint16_t dport_range;
	uint16_t precedence;
	uint8_t tos;
	uint8_t tos_mask;
	uint8_t xport_protocol;
	uint8_t filter_mask;
	uint8_t pad1;
	uint8_t pad2;
};

/*===========================================================================
			 DEFINITIONS AND DECLARATIONS
===========================================================================*/
typedef struct rmnetctl_hndl_s rmnetctl_hndl_t;

/*!
* @brief Public API to initialize the RMNET control driver
* @details Allocates memory for the RmNet handle. Creates and binds to a   and
* netlink socket if successful
* @param **rmnetctl_hndl_t_val RmNet handle to be initialized
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnetctl_init(rmnetctl_hndl_t **hndl, uint16_t *error_code);

/*!
* @brief Public API to clean up the RmNeT control handle
* @details Close the socket and free the RmNet handle
* @param *rmnetctl_hndl_t_val RmNet handle to be initialized
* @return void
*/
void rmnetctl_cleanup(rmnetctl_hndl_t *hndl);

/*!
* @brief Public API to register/unregister a RMNET driver on a particular device
* @details Message type is RMNET_NETLINK_ASSOCIATE_NETWORK_DEVICE or
* RMNET_NETLINK_UNASSOCIATE_NETWORK_DEVICE based on the flag for assoc_dev
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param dev_name Device on which to register the RmNet driver
* @param error_code Status code of this operation
* @param assoc_dev registers the device if RMNETCTL_DEVICE_ASSOCIATE or
* unregisters the device if RMNETCTL_DEVICE_UNASSOCIATE
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_associate_network_device(rmnetctl_hndl_t *hndl,
				   const char *dev_name,
				   uint16_t *error_code,
				   uint8_t assoc_dev);

/*!
* @brief Public API to get if a RMNET driver is registered on a particular
* device
* @details Message type is RMNET_NETLINK_GET_NETWORK_DEVICE_ASSOCIATED.
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param dev_name Device on which to check if the RmNet driver is registered
* @param register_status 1 if RmNet data driver is registered on a particular
* device, 0 if not
* @param error_code Status code of this operation
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_get_network_device_associated(rmnetctl_hndl_t *hndl,
					const char *dev_name,
					int *register_status,
					uint16_t *error_code);

/*!
* @brief Public API to set the egress data format for a particular link.
* @details Message type is RMNET_NETLINK_SET_LINK_EGRESS_DATA_FORMAT.
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param egress_flags Egress flags to be set on the device
* @param agg_size Max size of aggregated packets
* @param agg_count Number of packets to be aggregated
* @param dev_name Device on which to set the egress data format
* @param error_code Status code of this operation returned from the kernel
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_set_link_egress_data_format(rmnetctl_hndl_t *hndl,
				      uint32_t egress_flags,
				      uint16_t agg_size,
				      uint16_t agg_count,
				      const char *dev_name,
				      uint16_t *error_code);

/*!
* @brief Public API to get the egress data format for a particular link.
* @details Message type is RMNET_NETLINK_GET_LINK_EGRESS_DATA_FORMAT.
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param dev_name Device on which to get the egress data format
* @param egress_flags Egress flags from the device
* @param agg_count Number of packets to be aggregated
* @param error_code Status code of this operation returned from the kernel
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_get_link_egress_data_format(rmnetctl_hndl_t *hndl,
				      const char *dev_name,
				      uint32_t *egress_flags,
				      uint16_t *agg_size,
				      uint16_t *agg_count,
				      uint16_t *error_code);

/*!
* @brief Public API to set the ingress data format for a particular link.
* @details Message type is RMNET_NETLINK_SET_LINK_INGRESS_DATA_FORMAT.
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param ingress_flags Ingress flags from the device
* @param tail_spacing Tail spacing needed for the packet
* @param dev_name Device on which to set the ingress data format
* @param error_code Status code of this operation returned from the kernel
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_set_link_ingress_data_format_tailspace(rmnetctl_hndl_t *hndl,
						 uint32_t ingress_flags,
						 uint8_t  tail_spacing,
						 const char *dev_name,
						 uint16_t *error_code);

/*!
* @brief Public API to get the ingress data format for a particular link.
* @details Message type is RMNET_NETLINK_GET_LINK_INGRESS_DATA_FORMAT.
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param dev_name Device on which to get the ingress data format
* @param ingress_flags Ingress flags from the device
* @param tail_spacing Tail spacing needed for the packet
* @param error_code Status code of this operation returned from the kernel
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_get_link_ingress_data_format_tailspace(rmnetctl_hndl_t *hndl,
						const char *dev_name,
						uint32_t *ingress_flags,
						uint8_t  *tail_spacing,
						uint16_t *error_code);

inline int rmnet_set_link_ingress_data_format(rmnetctl_hndl_t *hndl,
					      uint32_t ingress_flags,
					      const char *dev_name,
					      uint16_t *error_code)
{
	return rmnet_set_link_ingress_data_format_tailspace(hndl,
							    ingress_flags,
							    0,
							    dev_name,
							    error_code);
}

inline int rmnet_get_link_ingress_data_format(rmnetctl_hndl_t *hndl,
					      const char *dev_name,
					      uint32_t *ingress_flags,
					      uint16_t *error_code)
{
	return rmnet_get_link_ingress_data_format_tailspace(hndl,
							    dev_name,
							    ingress_flags,
							    0,
							    error_code);
}

/*!
* @brief Public API to set the logical endpoint configuration for a
* particular link.
* @details Message type is RMNET_NETLINK_SET_LOGICAL_EP_CONFIG.
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param logical_ep_id Logical end point id on which the configuration is to be
* set
* @param rmnet_mode RmNet mode to be set on the device
* @param dev_name Device on which to set the logical end point configuration
* @param egress_dev_name Egress Device if operating in bridge mode
* @param error_code Status code of this operation returned from the kernel
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_set_logical_ep_config(rmnetctl_hndl_t *hndl,
				int32_t ep_id,
				uint8_t operating_mode,
				const char *dev_name,
				const char *next_dev,
				uint16_t *error_code);

/*!
* @brief Public API to un-set the logical endpoint configuration for a
* particular link.
* @details Message type is RMNET_NETLINK_UNSET_LOGICAL_EP_CONFIG.
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param logical_ep_id Logical end point id on which the configuration is to be
* un-set
* @param dev_name Device on which to un-set the logical end point configuration
* @param error_code Status code of this operation returned from the kernel
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_unset_logical_ep_config(rmnetctl_hndl_t *hndl,
				  int32_t ep_id,
				  const char *dev_name,
				  uint16_t *error_code);
/*!
* @brief Public API to get the logical endpoint configuration for a
* particular link.
* @details Message type is RMNET_NETLINK_GET_LOGICAL_EP_CONFIG.
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param logical_ep_id Logical end point id from which to get the configuration
* @param dev_name Device on which to get the logical end point configuration
* @param rmnet_mode RmNet mode from the device
* @param next_dev Egress Device name
* @param next_dev_len Egress Device I/O string len
* @param error_code Status code of this operation returned from the kernel
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_get_logical_ep_config(rmnetctl_hndl_t *hndl,
				int32_t ep_id,
				const char *dev_name,
				uint8_t *operating_mode,
				char **next_dev,
				uint32_t next_dev_len,
				uint16_t *error_code);

/*!
* @brief Public API to create a new virtual device node
* @details Message type is RMNET_NETLINK_NEW_VND or
* RMNETCTL_FREE_VND based on the flag for new_vnd
* @param hndl RmNet handle for the Netlink message
* @param id Node number to create the virtual network device node
* @param error_code Status code of this operation returned from the kernel
* @param new_vnd creates a new virtual network device if  RMNETCTL_NEW_VND or
* frees the device if RMNETCTL_FREE_VND
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_new_vnd(rmnetctl_hndl_t *hndl,
		  uint32_t id,
		  uint16_t *error_code,
		  uint8_t new_vnd);

/*!
 * @brief Public API to create a new virtual device node with a custom prefix
 * @details Message type is RMNET_NETLINK_NEW_VND or
 * RMNETCTL_FREE_VND based on the flag for new_vnd
 * @param hndl RmNet handle for the Netlink message
 * @param id Node number to create the virtual network device node
 * @param error_code Status code of this operation returned from the kernel
 * @param new_vnd creates a new virtual network device if  RMNETCTL_NEW_VND or
 * frees the device if RMNETCTL_FREE_VND
 * @param prefix Prefix to be used when naming the network interface
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rmnet_new_vnd_prefix(rmnetctl_hndl_t *hndl,
			 uint32_t id,
			 uint16_t *error_code,
			 uint8_t new_vnd,
			 const char *prefix);

/*!
 * @brief Public API to create a new virtual device node with a custom prefix
 * @details Message type is RMNET_NETLINK_NEW_VND or
 * RMNETCTL_FREE_VND based on the flag for new_vnd
 * @param hndl RmNet handle for the Netlink message
 * @param id Node number to create the virtual network device node
 * @param error_code Status code of this operation returned from the kernel
 * @param new_vnd creates a new virtual network device if  RMNETCTL_NEW_VND or
 * frees the device if RMNETCTL_FREE_VND
 * @param name Name to be used when naming the network interface
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rmnet_new_vnd_name(rmnetctl_hndl_t *hndl,
			 uint32_t id,
			 uint16_t *error_code,
			 const char *name);

/*!
 * @brief API to get the ASCII name of a virtual network device from its ID
 * @param hndl RmNet handle for the Netlink message
 * @param id Node number to create the virtual network device node
 * @param error_code Status code of this operation returned from the kernel
 * @param buf Buffer to store ASCII representation of device name
 * @param buflen Length of the buffer
 * @param prefix Prefix to be used when naming the network interface
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */

int rmnet_get_vnd_name(rmnetctl_hndl_t *hndl,
                      uint32_t id,
                      uint16_t *error_code,
                      char *buf,
                      uint32_t buflen);

/*!
* @brief Public API to set or clear a flow
* @details Message type is RMNET_NETLINK_ADD_VND_TC_FLOW or
* RMNET_NETLINK_DEL_VND_TC_FLOW based on the flag for set_flow
* @param *rmnetctl_hndl_t_val RmNet handle for the Netlink message
* @param id Node number to set or clear the flow on the virtual network
* device node
* @param map_flow_id Flow handle of the modem
* @param tc_flow_id Software flow handle
* @param set_flow sets the flow if  RMNET_NETLINK_SET_FLOW or
* clears the flow if RMNET_NETLINK_CLEAR_FLOW
* @return RMNETCTL_SUCCESS if successful
* @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
* @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
* Check error_code
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int rmnet_add_del_vnd_tc_flow(rmnetctl_hndl_t *hndl,
			      uint32_t id,
			      uint32_t map_flow_id,
			      uint32_t tc_flow_id,
			      uint8_t set_flow,
			      uint16_t *error_code);

/* @brief Public API to initialize the RTM_NETLINK RMNET control driver
 * @details Allocates memory for the RmNet handle. Creates and binds to a
 * netlink socket if successful
 * @param **rmnetctl_hndl_t_val RmNet handle to be initialized
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_init(rmnetctl_hndl_t **hndl, uint16_t *error_code);

/* @brief Public API to clean up the RTM_NETLINK RmNeT control handle
 * @details Close the socket and free the RmNet handle
 * @param *rmnetctl_hndl_t_val RmNet handle to be initialized
 * @return void
 */
int rtrmnet_ctl_deinit(rmnetctl_hndl_t *hndl);

/* @brief Public API to create a new virtual device node
 * @details Message type is RTM_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param dev_name Device name new node will be connected to
 * @param vnd_name Name of virtual device to be created
 * @param error_code Status code of this operation returned from the kernel
 * @param index Index node will have
 * @param flagconfig Flag configuration device will have
 * @param offload Offload capability
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_newvnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
		       uint16_t *error_code, uint8_t  index,
		       uint32_t flagconfig, uint8_t offload);

/* @brief Public API to delete a virtual device node
 * @details Message type is RTM_DELLINK
 * @param hndl RmNet handle for the Netlink message
 * @param vnd_name Name of virtual device to be deleted
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_delvnd(rmnetctl_hndl_t *hndl, char *vndname,
		       uint16_t *error_code);

/* @brief Public API to change flag's of a virtual device node
 * @details Message type is RTM_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param dev_name Name of device node is connected to
 * @param vnd_name Name of virtual device to be changed
 * @param error_code Status code of this operation returned from the kernel
 * @param flagconfig New flag config vnd should have
 * @param offload Offload capability
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_changevnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
			  uint16_t *error_code, uint8_t  index,
			  uint32_t flagconfig, uint8_t offload);

/* @brief Public API to retrieve configuration of a virtual device node
 * @details Message type is RTM_GETLINK
 * @param hndl RmNet handle for the Netlink message
 * @param vndname Name of virtual device to query
 * @param error_code Status code of this operation returned from the kernel
 * @param mux_id Where to store the value of the node's mux id
 * @param flagconfig Where to store the value of the node's data format flags
 * @param agg_count Where to store the value of the node's maximum packet count
 * for uplink aggregation
 * @param agg_size Where to store the value of the node's maximum byte count
 * for uplink aggregation
 * @param agg_time Where to store the value of the node's maximum time limit
 * for uplink aggregation
 * @param agg_time Where to store the value of the node's features
 * for uplink aggregation
 * @param offload Offload capability
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARF if invalid arguments were passed to the API
 */
int rtrmnet_ctl_getvnd(rmnetctl_hndl_t *hndl, char *vndname,
		       uint16_t *error_code, uint16_t *mux_id,
		       uint32_t *flagconfig, uint8_t *agg_count,
		       uint16_t *agg_size, uint32_t *agg_time,
		       uint8_t *features, uint8_t *offload);

/* @brief Public API to bridge a vnd and device
 * @details Message type is RTM_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param dev_name device to bridge msg will be sent to
 * @param vnd_name vnd name of device that will be dev_name's master
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMNETCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_ctl_bridgevnd(rmnetctl_hndl_t *hndl, char *devname, char *vndname,
			  uint16_t *error_code);

/* @brief Public API to configure the uplink aggregation parameters
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param packet_count Maximum number of packets to aggregate
 * @param byte_count Maximum number of bytes to aggregate
 * @param time_limit Maximum time to aggregate
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_set_uplink_aggregation_params(rmnetctl_hndl_t *hndl,
					  char *devname,
					  char *vndname,
					  uint8_t packet_count,
					  uint16_t byte_count,
					  uint32_t time_limit,
					  uint8_t features,
					  uint16_t *error_code);

/* @brief Public API to add flow control information
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param bearer_id Bearer information
 * @param flow_id Flow information
 * @param ip_type IP family
 * @param tcm_handle Flow related information
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_activate_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint32_t flow_id,
			  int ip_type,
			  uint32_t tcm_handle,
			  uint16_t *error_code);

/* @brief Public API to delete flow control information
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param bearer_id Bearer information
 * @param flow_id Flow information
 * @param ip_type IP family
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_delete_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint32_t flow_id,
			  int ip_type,
			  uint16_t *error_code);

/* @brief Public API to specify flow control information
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param bearer_id Bearer information
 * @param sequence Sequence id
 * @param grantsize Number of grant bytes
 * @param ack Acknowledgement indication
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_control_flow(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint8_t bearer_id,
			  uint16_t sequence,
			  uint32_t grantsize,
			  uint8_t ack,
			  uint16_t *error_code);

/* @brief Public API to specify flow state down information
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param bearer_id Bearer information
 * @param sequence Sequence id
 * @param grantsize Number of grant bytes
 * @param ack Acknowledgement indication
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_flow_state_down(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t instance,
			  uint16_t *error_code);

/* @brief Public API to specify flow state up information
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param instance Instance information
 * @param ep_type Endpoint type
 * @param ifaceid Endpoint id
 * @param flags Flags
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_flow_state_up(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t instance,
			  uint32_t ep_type,
			  uint32_t ifaceid,
			  int flags,
			  uint16_t *error_code);

/* @brief Public API to specify acknowledgement scaling
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param scale Acknowledgement scaling
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_set_qmi_scale(rmnetctl_hndl_t *hndl,
			  char *devname,
			  char *vndname,
			  uint32_t scale,
			  uint16_t *error_code);

/* @brief Public API to specify powersave polling information
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param freq Powersave polling frequency
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_set_wda_freq(rmnetctl_hndl_t *hndl,
			 char *devname,
			 char *vndname,
			 uint32_t freq,
			 uint16_t *error_code);

/* @brief Public API to add filter for a flow
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param flow_id Flow information
 * @param ip_type IP family
 * @param filter RMNETCTL filter
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_add_filter(rmnetctl_hndl_t *hndl,
		       char *devname,
		       char *vndname,
		       uint32_t flow_id,
		       int ip_type,
		       struct rmnetctl_filter *filter,
		       uint16_t *error_code);

/* @brief Public API to delete filters for a flow
 * used by the RmNet driver
 * @details Message type is RMN_NEWLINK
 * @param hndl RmNet handle for the Netlink message
 * @param devname Name of device node is connected to
 * @param vndname Name of virtual device
 * @param flow_id Flow information
 * @param ip_type IP family
 * @param error_code Status code of this operation returned from the kernel
 * @return RMNETCTL_SUCCESS if successful
 * @return RMENTCTL_LIB_ERR if there was a library error. Check error_code
 * @return RMNETCTL_KERNEL_ERR if there was an error in the kernel.
 * Check error_code
 * @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
 */
int rtrmnet_remove_filters(rmnetctl_hndl_t *hndl,
			   char *devname,
			   char *vndname,
			   uint32_t flow_id,
			   int ip_type,
			   uint16_t *error_code);

#endif /* not defined LIBRMNETCTL_H */

