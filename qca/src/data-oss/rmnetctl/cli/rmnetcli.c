/******************************************************************************

			R M N E T C L I . C

Copyright (c) 2013-2015, 2018, 2020 The Linux Foundation. All rights reserved.

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

/******************************************************************************

  @file    rmnetcli.c
  @brief   command line interface to expose rmnet control API's

  DESCRIPTION
  File containing implementation of the command line interface to expose the
  rmnet control configuration .

******************************************************************************/

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
#include "rmnetcli.h"
#include "librmnetctl.h"

#define RMNET_MAX_STR_LEN  16

#define _RMNETCLI_CHECKNULL(X)		do { if (!X) {                         \
print_rmnet_api_status(RMNETCTL_INVALID_ARG, RMNETCTL_CFG_FAILURE_NO_COMMAND); \
				rmnetctl_cleanup(handle);                      \
				return RMNETCTL_INVALID_ARG;                   \
		} } while (0);
#define _STRTOUI32(X)           (uint32_t)strtoul(X, NULL, 0)
#define _STRTOUI16(X)           (uint16_t)strtoul(X, NULL, 0)
#define _STRTOUI8(X)           (uint8_t)strtoul(X, NULL, 0)
#define _STRTOI32(X)           (int32_t)strtol(X, NULL, 0)

#define _5TABS 		"\n\t\t\t\t\t"
#define _2TABS 		"\n\t\t"

/*!
* @brief Contains a list of error message from CLI
*/
char rmnetcfg_error_code_text
[RMNETCFG_TOTAL_ERR_MSGS][RMNETCTL_ERR_MSG_SIZE] = {
	"Help option Specified",
	"ERROR: No\\Invalid command was specified\n",
	"ERROR: Could not allocate buffer for Egress device\n"
};

/*!
* @brief Method to display the syntax for the commands
* @details Displays the syntax and usage for the commands
* @param void
* @return void
*/
static void rmnet_api_usage(void)
{
	printf("RmNet API Usage:\n\n");
	printf("rmnetcli help                            Displays this help\n");
	printf("\n");
	printf("rmnetcli assocnetdev <dev_name>          Registers the RmNet");
	printf(_5TABS" data driver on a particular");
	printf(_5TABS" device.dev_name cannot");
	printf(_5TABS" be larger than 15");
	printf(_5TABS" characters. Returns");
	printf(_5TABS" the status code.\n\n");
	printf("rmnetcli unassocnetdev <dev_name>        Unregisters the");
	printf(_5TABS" RmNet data driver on a particular");
	printf(_5TABS" device. dev_name cannot");
	printf(_5TABS" be larger than 15");
	printf(_5TABS" characters. Returns");
	printf(_5TABS" the status code.\n\n");
	printf("rmnetcli getnetdevassoc <dev_name>       Get if the RmNet");
	printf(_5TABS" data driver is registered on");
	printf(_5TABS" a particular device.");
	printf(_5TABS" dev_name cannot be");
	printf(_5TABS" larger than 15");
	printf(_5TABS" characters. Returns 1");
	printf(_5TABS" if is registered and");
	printf(_5TABS" 0 if it is not");
	printf(_5TABS" registered\n\n");
	printf("rmnetcli setledf <egress_flags>          Sets the egress data");
	printf(_2TABS" <agg_size>              format for a particular link.");
	printf(_2TABS" <agg_count>             dev_name cannot be larger");
	printf(_2TABS" <dev_name>              than 15 characters.");
	printf(_5TABS" Returns the status code\n\n");
	printf("rmnetcli getledf <dev_name>              Gets the egress data");
	printf(_5TABS" format for a particular link.");
	printf(_5TABS" dev_name cannot be larger");
	printf(_5TABS" than 15. Returns the 4");
	printf(_5TABS" byte unsigned integer");
	printf(_5TABS" egress_flags\n\n");
	printf("rmnetcli setlidf <ingress_flags>         Sets the ingress");
	printf(_2TABS" <tail_spacing>          data format for a particular");
	printf(_2TABS" <dev_name>              link. ingress_flags is 4");
	printf(_5TABS" byte unsigned integer.");
	printf(_5TABS" tail_spacing is a one.");
	printf(_5TABS" byte unsigned integer.");
	printf(_5TABS" dev_name cannot be");
	printf(_5TABS" larger than 15.");
	printf(_5TABS" characters. Returns");
	printf(_5TABS" the status code\n\n");
	printf("rmnetcli getlidf <dev_name>              Gets the ingress");
	printf(_5TABS" data format for a particular");
	printf(_5TABS" link. dev_name cannot be");
	printf(_5TABS" larger than 15. Returns");
	printf(_5TABS" the 4 byte unsigned");
	printf(_5TABS" integer ingress_flags\n\n");
	printf("rmnetcli setlepc <logical_ep_id>         Sets the logical");
	printf(_2TABS" <rmnet_mode>            endpoint configuration for");
	printf(_2TABS" <dev_name>              a particular link.");
	printf(_2TABS" <egress_dev_name>       logical_ep_id are 32bit");
	printf(_5TABS" integers from -1 to 31.");
	printf(_5TABS" rmnet_mode is a 1 byte");
	printf(_5TABS" unsigned integer of");
	printf(_5TABS" value none, vnd or");
	printf(_5TABS" bridged. dev_name");
	printf(_5TABS" and egress_dev_name");
	printf(_5TABS" cannot be larger");
	printf(_5TABS" than 15 characters");
	printf(_5TABS" Returns the status code\n\n");
	printf("rmnetcli unsetlepc <logical_ep_id>       Un-sets the logical");
	printf(_2TABS"  <dev_name>              endpoint configuration for");
	printf(_5TABS" a particular link.");
	printf(_5TABS" integers from -1 to 31.");
	printf(_5TABS" dev_name cannot be larger");
	printf(_5TABS" than 15 characters");
	printf(_5TABS" Returns the status code\n\n");
	printf("rmnetcli getlepc <logical_ep_id>         Sets the logical");
	printf(_2TABS" <dev_name>              endpoint configuration for a");
	printf(_5TABS" particular link.");
	printf(_5TABS" logical_ep_id are 32bit");
	printf(_5TABS" integers from -1 to 31.");
	printf(_5TABS" Returns the rmnet_mode");
	printf(_5TABS" and egress_dev_name.");
	printf(_5TABS" rmnet_mode is a 1");
	printf(_5TABS" byte unsigned integer");
	printf(_5TABS" of value none, vnd or");
	printf(_5TABS" bridged. dev_name and");
	printf(_5TABS" egress_dev_name cannot be");
	printf(_5TABS" larger than 15 ");
	printf(_5TABS" characters. Returns the");
	printf(_5TABS" status code\n\n");
	printf("rmnetcli newvnd <dev_id>                 Creates a new");
	printf(_5TABS" virtual network device node.");
	printf(_5TABS" dev_id is an int");
	printf(_5TABS" less than 32. Returns");
	printf(_5TABS" the status code\n\n");
	printf("rmnetcli newvndprefix <dev_id> <name_prefix>   Creates");
	printf(_5TABS" virtual network device node.");
	printf(_5TABS" dev_id is an int");
	printf(_5TABS" less than 32. Prefix");
	printf(_5TABS" must be less than");
	printf(_5TABS" 15 chars. Returns");
	printf(_5TABS" the status code\n\n");
	printf("rmnetcli newvndname <dev_id> <name_prefix>   Creates");
	printf(_5TABS" virtual network device node.");
	printf(_5TABS" dev_id is an int");
	printf(_5TABS" less than 32. Name");
	printf(_5TABS" must be less than");
	printf(_5TABS" 15 chars. Returns");
	printf(_5TABS" the status code\n\n");
	printf("rmnetcli getvndname <dev_id>              Get name of");
	printf(_5TABS" network device node from id\n\n");
	printf("rmnetcli freevnd <dev_id>              Removes virtual");
	printf(_5TABS" network device node. dev_name");
	printf(_5TABS" cannot be larger than 15.");
	printf(_5TABS" Returns the status code\n\n");
	printf("rmnetcli addvnctcflow <dev_id>            Add a modem flow");
	printf(_2TABS" <mdm_flow_hndl>         handle - tc flow handle");
	printf(_2TABS" <tc_flow_hndl>          mapping for a virtual network");
	printf(_2TABS" device node\n\n");
	printf("rmnetcli delvnctcflow <dev_id>            Delete a modem flow");
	printf(_2TABS" <mdm_flow_hndl>         handle - tc flow handle");
	printf(_2TABS" <tc_flow_hndl>          mapping for a virtual network");
	printf(_2TABS" device node\n\n");
	printf("**************************\n");
	printf("RmNet RTM_NETLINK API Usage:\n\n");
	printf("rmnetcli -n newlink  <dev_id>            Add a vnd w/ newlink");
	printf(_2TABS" <vnd>                   string - vnd device_name");
	printf(_2TABS" <vnd id>                int - new vnd id");
	printf(_2TABS" [flags]                 int - starting flag config\n\n");
	printf("rmnetcli -n changelink  <dev_id>         Change a vnd's flags");
	printf(_2TABS" <vnd>                   string - vnd device_name");
	printf(_2TABS" <vnd id>                int - new vnd id");
	printf(_2TABS" <flags>                 int - new flag config\n\n");
	printf("rmnetcli -n getlink <dev_name>           Get device config\n\n");
	printf("rmnetcli -n dellink <dev_name>           Delete a vnd");
	printf(_2TABS"                         by inputting dev name\n\n");
	printf("rmnetcli -n bridgelink  <dev_name>       Bridge a vnd and a dev");
	printf(_2TABS" <vnd id>                by specifying dev id and vnd id\n\n");
	printf("rmnetcli -n uplinkparam <dev_name>   set uplink aggregation parameters");
	printf(_2TABS" <vnd id>                string - vnd device_name");
	printf(_2TABS" <packet count>          int - maximum packet count");
	printf(_2TABS" <byte count>            int - maximum byte count");
	printf(_2TABS" <time limit>            int - maximum time limit");
	printf(_2TABS" <features>              int - aggregation features\n\n");
	printf("rmnetcli -n flowactivate <real dev>  activate a flow\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <bearer_id>             int - bearer id\n\n");
	printf(_2TABS" <flow id>               int - flow id\n\n");
	printf(_2TABS" <ip type>               int - ip type\n\n");
	printf(_2TABS" <handle>                int - flow handle\n\n");
	printf("rmnetcli -n flowdel      <real dev> delete a flow\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <bearer_id>              int - bearer id\n\n");
	printf(_2TABS" <flow id>               int - flow id\n\n");
	printf(_2TABS" <ip type>               int - ip type\n\n");
	printf("rmnetcli -n flowcontrol  <real dev>");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <bearer_id>             int - bearer id\n\n");
	printf(_2TABS" <seq>                   int - sequence\n\n");
	printf(_2TABS" <grant size>            int - grant size\n\n");
	printf(_2TABS" <ack>                   int - ack\n\n");
	printf("rmnetcli -n systemup      <real dev>\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <instance>              int - bearer id\n\n");
	printf(_2TABS" <eptype>                int - ep type\n\n");
	printf(_2TABS" <iface_id>              int - iface id\n\n");
	printf(_2TABS" <flags>                 int - flags\n\n");
	printf("rmnetcli -n systemdown    <real dev> <vnd name> <instance>\n\n ");
	printf("rmnetcli -n qmiscale      <real dev> set ACK scale factor\n\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <scale>                 int - scale factor\n\n");
	printf("rmnetcli -n wdafreq       <real dev> set powersave poll freq\n\n");
	printf(_2TABS" <vnd_name>              string - vnd device name\n\n");
	printf(_2TABS" <freq>                  int - frequency\n\n");

}

static void print_rmnetctl_lib_errors(uint16_t error_number)
{
	if ((error_number > RMNETCTL_API_SUCCESS) &&
		(error_number < RMNETCTL_API_ERR_ENUM_LENGTH)) {
		printf("%s", rmnetctl_error_code_text[error_number]);
	}
	if ((error_number >= RMNETCFG_ERR_NUM_START) &&
	(error_number < RMNETCFG_ERR_NUM_START + RMNETCFG_TOTAL_ERR_MSGS)) {
		printf("%s", rmnetcfg_error_code_text
			[error_number - RMNETCFG_ERR_NUM_START]);
		if ((error_number == RMNETCTL_CFG_SUCCESS_HELP_COMMAND) ||
			(error_number == RMNETCTL_CFG_FAILURE_NO_COMMAND))
			rmnet_api_usage();
	}
}

/*!
* @brief Method to check the error numbers generated from API calls
* @details Displays the error messages based on each error code
* @param error_number Error number returned from the API and the CLI
* @return void
*/
static void print_rmnet_api_status(int return_code, uint16_t error_number)
{
	if (return_code == RMNETCTL_SUCCESS)
		printf("SUCCESS\n");
	else if (return_code == RMNETCTL_LIB_ERR) {
		printf("LIBRARY ");
		print_rmnetctl_lib_errors(error_number);
	} else if (return_code == RMNETCTL_KERNEL_ERR) {
		if (error_number < RMNETCTL_API_ERR_ENUM_LENGTH)
			printf("KERNEL ERROR: System or rmnet error %d\n",
			       error_number);
	}
	else if (return_code == RMNETCTL_INVALID_ARG)
		printf("INVALID_ARG\n");
}

/*!
* @brief Method to make the API calls
* @details Checks for each type of parameter and calls the appropriate
* function based on the number of parameters and parameter type
* @param argc Number of arguments which vary based on the commands
* @param argv Value of the arguments which vary based on the commands
* @return RMNETCTL_SUCCESS if successful. Relevant data might be printed
* based on the message type
* @return RMNETCTL_LIB_ERR if there was a library error. Error code will be
* printed
* @return RMNETCTL_KERNEL_ERR if there was a error in the kernel. Error code will be
* printed
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/

static int rmnet_api_call(int argc, char *argv[])
{
	struct rmnetctl_hndl_s *handle = NULL;
	uint16_t error_number = RMNETCTL_CFG_FAILURE_NO_COMMAND;
	int return_code = RMNETCTL_LIB_ERR;
	int is_new_api = 0;

	if ((!argc) || (!*argv)) {
		print_rmnet_api_status(RMNETCTL_LIB_ERR,
		RMNETCTL_CFG_FAILURE_NO_COMMAND);
		return RMNETCTL_LIB_ERR;
	}
	if (!strcmp(*argv, "help")) {
		print_rmnet_api_status(RMNETCTL_LIB_ERR,
		RMNETCTL_CFG_SUCCESS_HELP_COMMAND);
		return RMNETCTL_LIB_ERR;
	}

	if (!strcmp(*argv, "-n")) {
		is_new_api = 1;
		return_code = rtrmnet_ctl_init(&handle, &error_number);
		if (return_code != RMNETCTL_SUCCESS) {
			print_rmnet_api_status(return_code, error_number);
			return RMNETCTL_LIB_ERR;
		}
		error_number = RMNETCTL_CFG_FAILURE_NO_COMMAND;
		return_code = RMNETCTL_LIB_ERR;
		argv++;
		argc--;
		if ((!argc) || (!*argv)) {
			print_rmnet_api_status(RMNETCTL_LIB_ERR,
			RMNETCTL_CFG_FAILURE_NO_COMMAND);
			return RMNETCTL_LIB_ERR;
		}
		if (!strcmp(*argv, "newlink")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);

			uint8_t offload = 0;
			uint32_t flags = 0;
			/* If optional flag was used pass it on*/
			if (argv[4])
				flags = _STRTOI32(argv[4]);

			if (argv[5])
				offload = _STRTOUI8(argv[5]);

			return_code = rtrmnet_ctl_newvnd(handle, argv[1],
							 argv[2],
							 &error_number,
							 _STRTOI32(argv[3]),
							 flags,
							 offload);
		} else if (!strcmp(*argv, "changelink")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);

			return_code = rtrmnet_ctl_changevnd(handle, argv[1],
							    argv[2],
							    &error_number,
							    _STRTOI32(argv[3]),
							    _STRTOI32(argv[4]),
							    _STRTOUI8(argv[5]));
		} else if (!strcmp(*argv, "getlink")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			uint32_t flags = 0;
			uint16_t mux_id = 0;
			uint8_t agg_count = 0;
			uint16_t agg_size = 0;
			uint32_t agg_time = 0;
			uint8_t features = 0;
			uint8_t offload = 0;

			return_code = rtrmnet_ctl_getvnd(handle, argv[1],
							 &error_number,
							 &mux_id, &flags,
							 &agg_count, &agg_size,
							 &agg_time, &features,
							 &offload);
			if (return_code == RMNETCTL_API_SUCCESS) {
				printf("Configuration for device %s:\n", argv[1]);
				printf("\tMux id: %d\n", mux_id);
				printf("\tData format: 0x%04x\n", flags);
				printf("\tUplink Aggregation parameters:\n");
				printf("\t\tPacket limit: %d\n", agg_count);
				printf("\t\tByte limit: %d\n", agg_size);
				printf("\t\tTime limit (ns): %d\n", agg_time);
				printf("\t\tFeatures : 0x%02x\n", features);
				printf("\tOffload : 0x%02x\n", offload);
			}
		} else if (!strcmp(*argv, "dellink")) {
			_RMNETCLI_CHECKNULL(argv[1]);
				return_code = rtrmnet_ctl_delvnd(handle, argv[1],
								 &error_number);
		} else if (!strcmp(*argv, "bridge")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			return_code = rtrmnet_ctl_bridgevnd(handle, argv[1],
							    argv[2],
							    &error_number);
		} else if (!strcmp(*argv, "uplinkparam")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);

			return_code = rtrmnet_set_uplink_aggregation_params(
				handle, argv[1], argv[2], _STRTOUI8(argv[3]),
				_STRTOUI16(argv[4]), _STRTOUI32(argv[5]),
				_STRTOUI8(argv[6]), &error_number);
		}
		else if (!strcmp(*argv, "flowactivate")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);

			return_code = rtrmnet_activate_flow(handle, argv[1], argv[2],
							    _STRTOUI8(argv[3]),
							    _STRTOI32(argv[4]),
							    _STRTOUI32(argv[5]),
							    _STRTOUI32(argv[6]),
							    &error_number);

		} else if (!strcmp(*argv, "flowdel")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			return_code = rtrmnet_delete_flow(handle, argv[1], argv[2],
							_STRTOUI8(argv[3]),
							_STRTOUI32(argv[4]),
							_STRTOI32(argv[5]),
							&error_number);
		} else if (!strcmp(*argv, "flowcontrol")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);


			return_code = rtrmnet_control_flow(handle, argv[1], argv[2],
							    _STRTOUI8(argv[3]),
							    _STRTOUI32(argv[4]),
							    _STRTOUI16(argv[5]),
							    _STRTOUI8(argv[6]),
							    &error_number);
		} else if (!strcmp(*argv, "systemup")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);
			_RMNETCLI_CHECKNULL(argv[4]);
			_RMNETCLI_CHECKNULL(argv[5]);
			_RMNETCLI_CHECKNULL(argv[6]);


			return_code = rtrmnet_flow_state_up(handle, argv[1], argv[2],
							    _STRTOUI32(argv[3]),
							    _STRTOUI32(argv[4]),
							    _STRTOUI32(argv[5]),
							    _STRTOUI32(argv[6]),
							    &error_number);
		} else if (!strcmp(*argv, "systemdown")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);

			return_code = rtrmnet_flow_state_down(handle, argv[1], argv[2],
							    _STRTOUI32(argv[3]),
							    &error_number);
		} else if (!strcmp(*argv, "qmiscale")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);

			return_code = rtrmnet_set_qmi_scale(handle, argv[1], argv[2],
							    _STRTOUI32(argv[3]),
							    &error_number);
		} else if (!strcmp(*argv, "wdafreq")) {
			_RMNETCLI_CHECKNULL(argv[1]);
			_RMNETCLI_CHECKNULL(argv[2]);
			_RMNETCLI_CHECKNULL(argv[3]);

			return_code = rtrmnet_set_wda_freq(handle, argv[1], argv[2],
							   _STRTOUI32(argv[3]),
							   &error_number);
		}


		goto end;
	} else {
		return_code = rmnetctl_init(&handle, &error_number);
		if (return_code != RMNETCTL_SUCCESS) {
			print_rmnet_api_status(return_code, error_number);
			return RMNETCTL_LIB_ERR;
		}

	}
	error_number = RMNETCTL_CFG_FAILURE_NO_COMMAND;
	return_code = RMNETCTL_LIB_ERR;
	if (!strcmp(*argv, "assocnetdev")) {
		return_code = rmnet_associate_network_device(handle,
		argv[1], &error_number, RMNETCTL_DEVICE_ASSOCIATE);
	} else if (!strcmp(*argv, "unassocnetdev")) {
		return_code = rmnet_associate_network_device(handle,
		argv[1], &error_number, RMNETCTL_DEVICE_UNASSOCIATE);
	} else if (!strcmp(*argv, "getnetdevassoc")) {
		int register_status;
		return_code = rmnet_get_network_device_associated(handle,
		argv[1], &register_status, &error_number);
		if (return_code == RMNETCTL_SUCCESS)
			printf("register_status is %d\n", register_status);
	} else if (!strcmp(*argv, "getledf")) {
		uint32_t egress_flags;
		uint16_t agg_size, agg_count;
		return_code = rmnet_get_link_egress_data_format(handle,
		argv[1], &egress_flags, &agg_size, &agg_count, &error_number);
		if (return_code == RMNETCTL_SUCCESS) {
			printf("egress_flags is %u\n", egress_flags);
			printf("agg_size is %u\n", agg_size);
			printf("agg_count is %u\n", agg_count);
		}
	} else if (!strcmp(*argv, "getlidf")) {
		uint32_t ingress_flags;
		uint8_t  tail_spacing;
		return_code = rmnet_get_link_ingress_data_format_tailspace(
		handle, argv[1], &ingress_flags, &tail_spacing, &error_number);
		if (return_code == RMNETCTL_SUCCESS) {
			printf("ingress_flags is %u\n", ingress_flags);
			printf("tail_spacing is %u\n", tail_spacing);
		}
	} else if (!strcmp(*argv, "newvndprefix")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		_RMNETCLI_CHECKNULL(argv[2]);
		return_code = rmnet_new_vnd_prefix(handle,
		_STRTOUI32(argv[1]), &error_number, RMNETCTL_NEW_VND, argv[2]);
	} else if (!strcmp(*argv, "newvndname")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		_RMNETCLI_CHECKNULL(argv[2]);
		return_code = rmnet_new_vnd_name(handle,
		_STRTOUI32(argv[1]), &error_number, argv[2]);
	} else if (!strcmp(*argv, "newvnd")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		return_code = rmnet_new_vnd(handle,
		_STRTOUI32(argv[1]), &error_number, RMNETCTL_NEW_VND);
	} else if (!strcmp(*argv, "getvndname")) {
		char buffer[32];
		memset(buffer, 0, 32);
		_RMNETCLI_CHECKNULL(argv[1]);
		return_code = rmnet_get_vnd_name(handle, _STRTOUI32(argv[1]),
			           &error_number, buffer, 32);
		if (return_code == RMNETCTL_SUCCESS) {
			printf("VND name: %s\n", buffer);
		}
	} else if (!strcmp(*argv, "freevnd")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		return_code = rmnet_new_vnd(handle,
		_STRTOUI32(argv[1]), &error_number, RMNETCTL_FREE_VND);
	} else if (!strcmp(*argv, "setlidf")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		_RMNETCLI_CHECKNULL(argv[2]);
		_RMNETCLI_CHECKNULL(argv[3]);
		return_code = rmnet_set_link_ingress_data_format_tailspace(
		handle, _STRTOUI32(argv[1]), _STRTOUI8(argv[2]), argv[3],
		&error_number);
	} else if (!strcmp(*argv, "delvnctcflow")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		_RMNETCLI_CHECKNULL(argv[2]);
		_RMNETCLI_CHECKNULL(argv[3]);
		return_code = rmnet_add_del_vnd_tc_flow(handle,
		_STRTOUI32(argv[1]), _STRTOUI32(argv[2]), _STRTOUI32(argv[3]),
		RMNETCTL_DEL_FLOW, &error_number);
	} else if (!strcmp(*argv, "getlepc")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		uint8_t rmnet_mode;
		char *egress_dev_name;
		egress_dev_name = NULL;
		egress_dev_name = (char *)malloc(RMNET_MAX_STR_LEN
		* sizeof(char));
		if (!egress_dev_name) {
			print_rmnet_api_status(RMNETCTL_LIB_ERR,
			RMNETCTL_CFG_FAILURE_EGRESS_DEV_NAME_NULL);
			rmnetctl_cleanup(handle);
			return RMNETCTL_LIB_ERR;
		}
		return_code = rmnet_get_logical_ep_config(handle,
		_STRTOI32(argv[1]), argv[2], &rmnet_mode,
		&egress_dev_name, RMNET_MAX_STR_LEN, &error_number);
		if (return_code == RMNETCTL_SUCCESS) {
			printf("rmnet_mode is %u\n", rmnet_mode);
			printf("egress_dev_name is %s\n", egress_dev_name);
		}
		free(egress_dev_name);
	} else if (!strcmp(*argv, "addvnctcflow")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		_RMNETCLI_CHECKNULL(argv[2]);
		_RMNETCLI_CHECKNULL(argv[3]);
		return_code = rmnet_add_del_vnd_tc_flow(handle,
		_STRTOUI32(argv[1]), _STRTOUI32(argv[2]), _STRTOUI32(argv[3]),
		RMNETCTL_ADD_FLOW, &error_number);
	} else if (!strcmp(*argv, "setledf")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		_RMNETCLI_CHECKNULL(argv[2]);
		_RMNETCLI_CHECKNULL(argv[3]);
		return_code = rmnet_set_link_egress_data_format(handle,
		_STRTOUI32(argv[1]), _STRTOUI16(argv[2]), _STRTOUI16(argv[3]),
		argv[4], &error_number);
	} else if (!strcmp(*argv, "setlepc")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		_RMNETCLI_CHECKNULL(argv[2]);
		return_code = rmnet_set_logical_ep_config(handle,
		_STRTOI32(argv[1]), _STRTOUI8(argv[2]), argv[3], argv[4],
		&error_number);
	} else if (!strcmp(*argv, "unsetlepc")) {
		_RMNETCLI_CHECKNULL(argv[1]);
		return_code = rmnet_unset_logical_ep_config(handle,
		_STRTOI32(argv[1]), argv[2], &error_number);
	}
end:
	print_rmnet_api_status(return_code, error_number);
	if (is_new_api)
		rtrmnet_ctl_deinit(handle);
	else
		rmnetctl_cleanup(handle);
	return return_code;
}

/*!
* @brief Method which serves as en entry point to the rmnetcli function
* @details Entry point for the RmNet Netlink API. This is the command line
* interface for the RmNet API
* @param argc Number of arguments which vary based on the commands
* @param argv Value of the arguments which vary based on the commands
* @return RMNETCTL_SUCCESS if successful. Relevant data might be printed
* based on the message type
* @return RMNETCTL_LIB_ERR if there was a library error. Error code will be
* printed
* @return RMNETCTL_KERNEL_ERR if there was a error in the kernel. Error code will be
* printed
* @return RMNETCTL_INVALID_ARG if invalid arguments were passed to the API
*/
int main(int argc, char *argv[])
{
	argc--;
	argv++;
	return rmnet_api_call(argc, argv);
}
