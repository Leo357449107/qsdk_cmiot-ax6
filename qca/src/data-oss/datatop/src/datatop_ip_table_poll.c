/************************************************************************
Copyright (c) 2016, The Linux Foundation. All rights reserved.

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
************************************************************************/

/**
 * @file datatop_ip_table_poll.c
 * @brief Adds ability for TP Tables, Rules and Routes data collection
          Unlike other polls, this is intended for running as a separate
          thread as it can cause delays of > 3sec per poll
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "datatop_interface.h"
#include "datatop_fileops.h"
#include "datatop_str.h"
#include "datatop_polling.h"

#define DTOP_IPTRR_POLL_PERIOD  5.00

/**
* @struct dtop_ip_table_vars
* @brief Struct used to hold necessary variables for /proc/stat dpg
*
* @var dtop_ip_table_vars::line
* Array of strings where necessary dp names and values are held.
* @var dtop_ip_table_vars::line_count
* Number of lines the file is that the dpg represents.
*/
struct dtop_ip_table_vars {
	char *out_dir;
}dtop_ip_table_storage;

struct dtop_linked_list *ip_dpg_list = NULL;

pthread_mutex_t dtop_ip_table_lock;

/**
 * @brief Perform IP table command and store it in a file
 *
 * @param dpg Struct that polled data is added to.
 * @return DTOP_POLL_IO_ERR - Poll of dpg unsuccessful.
 * @return DTOP_POLL_OK - Poll of dpg successful.
 */
int dtop_ip_table_poll(struct dtop_data_point_gatherer *dpg)
{
  FILE *fd;
  FILE *fo = (FILE *)dpg->file;
  char buf[1001];

  time_t rawtime;
  struct tm * timeinfo;

  if(fo == NULL)
  {
    fprintf(stderr, "Could not fopen: %s\n", dpg->file);
	  return DTOP_POLL_IO_ERR;
  }

  time ( &rawtime );
  timeinfo = gmtime ( &rawtime );

  fprintf ( fo, "============\nStart: %s==========\n", asctime (timeinfo) );
  fflush(fo);

  /* redirect stderr to output file */
  dup2(fileno(fo), 2);

  fd = popen((char *)dpg->priv, "r");
  if(fd == NULL)
  {
    fprintf(stderr, "Could not popen: %s\n", (char *)dpg->priv);
	  return DTOP_POLL_IO_ERR;
  }

  while(fgets(buf, 1000, fd) != NULL)
  {
    fputs(buf, fo);
  }

  fprintf ( fo, "============\nEnd: %s==========\n\n", asctime (timeinfo) );
  fflush(fo);
  pclose(fd);
	return DTOP_POLL_OK;
}

/**
 * @brief Frees dynamically allocated IP table dpg.
 *
 * Frees the memory of the dpg along with it's data_points
 * and other malloc'd memory no longer needed.
 *
 * @param dpg Dpg to deconstruct and deallocate memory for.
 */
static void dtop_ip_table_dpg_deconstructor
			(struct dtop_data_point_gatherer *dpset)
{
	int i;
	free(dpset->prefix);
  if(dpset->file)
  {
     fclose((FILE *)dpset->file);
  }

	free(dpset);
}

/**
 * @brief Registers a new IP table dpg to the list.
 *
 * @param dpg Dpg to construct and allocate memory for.
 */
void dtop_ip_table_register(struct dtop_data_point_gatherer *dpg)
{
  if (dpg)
    ip_dpg_list = dtop_add_linked_list(dpg, ip_dpg_list);
}

/**
 * @brief Open the files for writing the output for each dpg.
 *
 * @param None
 */
int dtop_ip_table_init_files()
{
  struct dtop_data_point_gatherer *dpset;
  struct dtop_linked_list *curr_ptr = ip_dpg_list;
  FILE *fd;

  while(curr_ptr)
  {
    dpset = (struct dtop_data_point_gatherer *) curr_ptr->data;
    fd = fopen(dpset->prefix, "a+");
    if(!fd)
    {
      fprintf(stderr, "Could not fopen: %s\n", dpset->prefix);
      return DTOP_POLL_IO_ERR;
    }
    dpset->file = (char *)fd;
    curr_ptr = curr_ptr->next_ptr;
  }
  return DTOP_POLL_OK;
}

/**
 * @brief Perform cleanup of IP table dgp list at exit.
 *
 * @param None
 */
void dtop_ip_table_poll_cleanup()
{
  struct dtop_data_point_gatherer *dpset;
  struct dtop_linked_list *curr_ptr = ip_dpg_list;

  pthread_mutex_lock(&dtop_ip_table_lock);
  deconstruct_dpgs(ip_dpg_list);
  dtop_rem_linked_list(ip_dpg_list);
  pthread_mutex_unlock(&dtop_ip_table_lock);

}

/**
 * @brief The thread to poll for IP table data.
 *
 * @param arg ptr
 */
void *dtop_ip_table_start_poll(void *arg)
{
  time_t start_t, curr_t;
  double diff_t = 9999999.00; /* some high # > DTOP_IPTRR_POLL_PERIOD */
  int ret = DTOP_POLL_OK;

  if (pthread_mutex_init(&dtop_ip_table_lock, NULL) != 0)
  {
      printf("\n mutex init failed\n");
      return NULL;
  }

  atexit(dtop_ip_table_poll_cleanup);

  if(DTOP_POLL_OK != ( ret = dtop_ip_table_init_files()))
  {
    return NULL;
  }

  while(1)
  {
    struct dtop_linked_list *curr_ptr = ip_dpg_list;
    struct dtop_data_point_gatherer *dpset;

    pthread_mutex_lock(&dtop_ip_table_lock);

    if (diff_t >= DTOP_IPTRR_POLL_PERIOD)
    {
      printf("Poll for IP Tables, Rules & Routes\n");
      time(&start_t);
      while (curr_ptr)
      {
        dpset = (struct dtop_data_point_gatherer *) curr_ptr->data;
        dpset->poll(dpset);
        curr_ptr = curr_ptr->next_ptr;
      }
    }
    pthread_mutex_unlock(&dtop_ip_table_lock);

    /* sleep for 500 milliseconds */
    usleep(500 * 1000);
    time(&curr_t);
    diff_t = difftime(curr_t, start_t);
  }
  return NULL;
}

/**
 * @brief Creates a dpg for ip table command
 *
 * Dynamically allocates memory for dpg which is then added to a linked list
 * via the dtop_register(dpg) function call.
 *
 * @param data_points dtop_data_point struct that dpg points to.
 * @param storage dtop_ip_table_vars struct that holds relevant dpg variables.
 */
/*static void construct_ip_table_dpg(struct dtop_data_point
		*data_points, struct dtop_ip_table_vars *command, int dp_count)
*/
static void construct_ip_table_dpg(char *command)
{
	struct dtop_data_point_gatherer *dpg = malloc
		(sizeof(struct dtop_data_point_gatherer));
  char *file_name = (char *)malloc(strlen(command)+ 1 + 1 + strlen(dtop_ip_table_storage.out_dir) + 4);
  int i, fname_start_ind;

  strcpy(file_name, dtop_ip_table_storage.out_dir);
  strcat(file_name, "/");

  fname_start_ind = strlen(file_name);
  strcat(file_name, command);
  strcat(file_name, ".txt");

  for(i=fname_start_ind; file_name[i]; i++)
  {
    if(file_name[i] == ' ')
      file_name[i] = '_';
    if(file_name[i] == '/')
      file_name[i] = '-';
  }

	dpg->prefix = file_name;
	dpg->poll = dtop_ip_table_poll;
	dpg->priv = (char *)command;
  dpg->file = NULL;
	dpg->deconstruct = dtop_ip_table_dpg_deconstructor;

	dtop_ip_table_register(dpg);
}

/*
 * @brief Scans "/proc/stat" in order to autodetect dps.
 *
 * Searches through "/proc/stat" file for all available data
 * points to create as dp structs.
 *
 * @param storage dtop_ip_table_vars struct where relevant variables are stored.
 */

/**
 * @brief Calls dtop_search for "/proc/stat" file.
 */
void dtop_ip_table_init(char *out_dir)
{
	dtop_ip_table_storage.out_dir = out_dir;
  construct_ip_table_dpg("ip xfrm state show");
  construct_ip_table_dpg("ip xfrm policy show");
  construct_ip_table_dpg("ip addr");
  construct_ip_table_dpg("iptables -t raw -L -n -v");
  construct_ip_table_dpg("iptables -t mangle -L -n -v");
  construct_ip_table_dpg("iptables -L -n -v");
  construct_ip_table_dpg("iptables -t nat -L -n -v");
  construct_ip_table_dpg("ip6tables -t raw -L -n -v");
  construct_ip_table_dpg("ip6tables -t mangle -L -n -v");
  construct_ip_table_dpg("ip6tables -L -n -v");
  construct_ip_table_dpg("ip6tables -t nat -L -n -v");
  construct_ip_table_dpg("ip rule show");
  construct_ip_table_dpg("ip -6 rule show");
  construct_ip_table_dpg("ip route show table all");
  construct_ip_table_dpg("ip -6 route show table all");
  construct_ip_table_dpg("ip route show table rmnet_data0");
  construct_ip_table_dpg("ip route show table rmnet_data1");
  construct_ip_table_dpg("ip route show table rmnet_data2");
  construct_ip_table_dpg("ip route show table rmnet_data6");
  construct_ip_table_dpg("ip route show table rmnet_data7");
  construct_ip_table_dpg("ip route show table r_rmnet_data0");
  construct_ip_table_dpg("ip route show table r_rmnet_data1");
  construct_ip_table_dpg("ip route show table r_rmnet_data2");
  construct_ip_table_dpg("ip route show table r_rmnet_data6");
  construct_ip_table_dpg("ip route show table r_rmnet_data7");
  construct_ip_table_dpg("ip -6 route show table rmnet_data0");
  construct_ip_table_dpg("ip -6 route show table rmnet_data1");
  construct_ip_table_dpg("ip -6 route show table rmnet_data2");
  construct_ip_table_dpg("ip -6 route show table rmnet_data6");
  construct_ip_table_dpg("ip -6 route show table rmnet_data7");
  construct_ip_table_dpg("ip -6 route show table r_rmnet_data0");
  construct_ip_table_dpg("ip -6 route show table r_rmnet_data1");
  construct_ip_table_dpg("ip -6 route show table r_rmnet_data2");
  construct_ip_table_dpg("ip -6 route show table r_rmnet_data6");
  construct_ip_table_dpg("ip -6 route show table r_rmnet_data7");
  construct_ip_table_dpg("ip route show table wlan0");
  construct_ip_table_dpg("ip -6 route show table wlan0");
  construct_ip_table_dpg("ip route show table dummy0");
  construct_ip_table_dpg("ip -6 route show table dummy0");
  construct_ip_table_dpg("cat /proc/net/xfrm_stat");
  construct_ip_table_dpg("cat /proc/sys/net/ipv4/ip_forward");
  construct_ip_table_dpg("cat /proc/sys/net/ipv6/conf/all/forwarding");

  printf("Poll for IP Tables, Rules & Routes every 5 seconds\n");
}
