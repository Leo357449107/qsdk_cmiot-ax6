/*
 **************************************************************************
 * Copyright (c) 2014,2015,2017-2020 The Linux Foundation. All rights reserved.
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all copies.
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **************************************************************************
 */

/*
 * Listener
 * Listeners are entities that monitor database events
 */
struct ecm_db_listener_instance;
typedef void (*ecm_db_listener_final_callback_t)(void *arg);		/* Finaliser callback */

/*
 * Interface
 * An interface represents an interface of this device to which nodes may be reached
 */
struct ecm_db_iface_instance;

/*
 * Node
 * A node instance is the ethernet representation of the host, i.e. the mac address to send packets to when reaching the host
 */
struct ecm_db_node_instance;

/*
 * Host
 * A host instance identifies a node by IP address
 */
struct ecm_db_host_instance;

/*
 * Host owner events
 */
typedef void (*ecm_db_host_final_callback_t)(void *arg);		/* Finaliser callback */

/*
 * Host listener events
 */
typedef void (*ecm_db_host_listener_added_callback_t)(void *arg, struct ecm_db_host_instance *hi);		/* Added callback */
typedef void (*ecm_db_host_listener_removed_callback_t)(void *arg, struct ecm_db_host_instance *hi);		/* Removed callback */

/*
 * Mapping
 * A mapping defines a port number.  Non-port based protocols will use -1 so they will all share to the same mapping instance.
 */
struct ecm_db_mapping_instance;

/*
 * Mapping owner events
 */
typedef void (*ecm_db_mapping_final_callback_t)(void *arg);		/* Finaliser callback */

/*
 * Mapping listener events
 */
typedef void (*ecm_db_mapping_listener_added_callback_t)(void *arg, struct ecm_db_mapping_instance *mi);		/* Added callback */
typedef void (*ecm_db_mapping_listener_removed_callback_t)(void *arg, struct ecm_db_mapping_instance *mi);	/* Removed callback */

/*
 * Node
 */
typedef void (*ecm_db_node_final_callback_t)(void *arg);		/* Finaliser callback */
typedef void (*ecm_db_node_listener_added_callback_t)(void *arg, struct ecm_db_node_instance *ni);		/* Added callback */
typedef void (*ecm_db_node_listener_removed_callback_t)(void *arg, struct ecm_db_node_instance *ni);		/* Removed callback */

/*
 * Interface
 */
typedef void (*ecm_db_iface_final_callback_t)(void *arg);		/* Finaliser callback */
typedef void (*ecm_db_iface_listener_added_callback_t)(void *arg, struct ecm_db_iface_instance *ii);		/* Added callback */
typedef void (*ecm_db_iface_listener_removed_callback_t)(void *arg, struct ecm_db_iface_instance *ii);		/* Removed callback */

/*
 * Time out values - in seconds - used to configure timer groups
 */
#define ECM_DB_CLASSIFIER_DETERMINE_GENERIC_TIMEOUT 5
#define ECM_DB_CONNECTION_GENERIC_TIMEOUT 240
#define ECM_DB_CONNECTION_TCP_RST_TIMEOUT 240
#define ECM_DB_CONNECTION_TCP_SHORT_TIMEOUT 240
#define ECM_DB_CONNECTION_TCP_CLOSED_TIMEOUT 10
#define ECM_DB_CONNECTION_TCP_LONG_TIMEOUT 3600
#define ECM_DB_CONNECTION_UDP_TIMEOUT 300
#define ECM_DB_CONNECTION_IGMP_TIMEOUT 240
#define ECM_DB_CONNECTION_ICMP_TIMEOUT 60
#define ECM_DB_CONNECTION_PPTP_DATA_TIMEOUT 300
#define ECM_DB_CONNECTION_RTCP_TIMEOUT 1800
#define ECM_DB_CONNECTION_RTSP_FAST_TIMEOUT 5
#define ECM_DB_CONNECTION_RTSP_SLOW_TIMEOUT 120
#define ECM_DB_CONNECTION_DNS_TIMEOUT 30
#define ECM_DB_CONNECTION_FTP_TIMEOUT 60
#define ECM_DB_CONNECTION_H323_TIMEOUT 28800
#define ECM_DB_CONNECTION_IKE_TIMEOUT 54000
#define ECM_DB_CONNECTION_ESP_TIMEOUT 7800
#define ECM_DB_CONNECTION_ESP_PENDING_TIMEOUT 3
#define ECM_DB_CONNECTION_SDP_TIMEOUT 120
#define ECM_DB_CONNECTION_SIP_TIMEOUT 28800
#define ECM_DB_CONNECTION_BITTORRENT_TIMEOUT 120
#define ECM_DB_CONNECTION_DEFUNCT_RETRY_TIMEOUT 5

/*
 * ECM database object direction.
 */
enum ecm_db_obj_dir {
	ECM_DB_OBJ_DIR_FROM,
	ECM_DB_OBJ_DIR_TO,
	ECM_DB_OBJ_DIR_FROM_NAT,
	ECM_DB_OBJ_DIR_TO_NAT,
	ECM_DB_OBJ_DIR_MAX
};
typedef enum ecm_db_obj_dir ecm_db_obj_dir_t;

/*
 * Extern decleration of common array that maps
 * the object direction to a string.
 */
extern char *ecm_db_obj_dir_strings[ECM_DB_OBJ_DIR_MAX];

/*
 * Timer groups.
 * WARNING: Only connections may use a connection timer group as these are subject to reaping.
 */
enum ecm_db_timer_groups {
	ECM_DB_TIMER_GROUPS_CLASSIFIER_DETERMINE_GENERIC_TIMEOUT = 0,
								/* Generic timeout for a classifier in determine phase */
	ECM_DB_TIMER_GROUPS_CONNECTION_GENERIC_TIMEOUT,		/* Generic timeout for a connection */
	ECM_DB_TIMER_GROUPS_CONNECTION_UDP_GENERIC_TIMEOUT,	/* Standard UDP Timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_UDP_WKP_TIMEOUT,		/* Standard UDP Timeout for all connections where at least one port is < 1024 */
	ECM_DB_TIMER_GROUPS_CONNECTION_ICMP_TIMEOUT,		/* Standard ICMP Timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_TCP_SHORT_TIMEOUT,	/* TCP Short timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_TCP_RESET_TIMEOUT,	/* TCP Reset timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_TCP_CLOSED_TIMEOUT,	/* TCP Closed timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_TCP_LONG_TIMEOUT,	/* TCP Long timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_PPTP_DATA_TIMEOUT,	/* PPTP Tunnel Data timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_RTCP_TIMEOUT,		/* RTCP timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_RTSP_TIMEOUT,		/* RTSP connection timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_RTSP_FAST_TIMEOUT,	/* RTSP fast static NAT connection timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_RTSP_SLOW_TIMEOUT,	/* RTSP slow static NAT connection timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_DNS_TIMEOUT,		/* DNS timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_FTP_TIMEOUT,		/* FTP connection timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_H323_TIMEOUT,		/* H323 timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_IKE_TIMEOUT,		/* IKE timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_ESP_TIMEOUT,		/* ESP timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_ESP_PENDING_TIMEOUT,	/* ESP Pending connection timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_SDP_TIMEOUT,		/* SDP timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_SIP_TIMEOUT,		/* SIP timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_IGMP_TIMEOUT,		/* IGMP timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_BITTORRENT_TIMEOUT,	/* Bittorrent connections timeout */
	ECM_DB_TIMER_GROUPS_CONNECTION_DEFUNCT_RETRY_TIMEOUT,	/* Defunct retry timeout */
	ECM_DB_TIMER_GROUPS_MAX					/* Always the last one */
};
typedef enum ecm_db_timer_groups ecm_db_timer_group_t;
typedef void (*ecm_db_timer_group_entry_callback_t)(void *arg);	/* Timer entry has expired */

/*
 * Ignore IP version check in connection instance
 */
#define ECM_DB_IP_VERSION_IGNORE 0

#ifdef ECM_MULTICAST_ENABLE

struct ecm_db_multicast_tuple_instance;

/*
 * Need to set this flag when Multicast is attached to ci and added to tuple hash table
 */
#define ECM_DB_MULTICAST_TUPLE_INSTANCE_FLAGS_INSERTED 0x1

/*
 *  This flag is used to find out whether a bridge device is present or not in a
 *  multicast destination interface list, struct ecm_db_multicast_connection_instance
 *  flags field use this flag.
 */
#define ECM_DB_MULTICAST_CONNECTION_BRIDGE_DEV_SET_FLAG 0x10

/*
 *  Maximum number of destination interfaces for
 *  a multicast connection
 */
#define ECM_DB_MULTICAST_IF_MAX 16

/*
 * struct ecm_multicast_if_update
 * 	This is used for storing processed updates to the destination
 * 	interface list of a multicast connection. The updates are
 * 	received from multicast routing or bridge snooper due to IGMP
 * 	join or leave events
 */
struct ecm_multicast_if_update {
	uint32_t if_join_cnt;
	uint32_t if_join_idx[ECM_DB_MULTICAST_IF_MAX];
	uint32_t join_dev[ECM_DB_MULTICAST_IF_MAX];
	uint32_t if_leave_cnt;
	uint32_t if_leave_idx[ECM_DB_MULTICAST_IF_MAX];
};
#endif

/*
 * struct ecm_db_timer_group_entry
 *	Time entry structure used to request timer service
 * WARNING: Do NOT inspect any of these fields - they are for exclusive use of the DB timer code.  Use the API's to control the timer and inspect its state.
 */
struct ecm_db_timer_group_entry {
	struct ecm_db_timer_group_entry *next;			/* Link to the next entry in the timer group chain */
	struct ecm_db_timer_group_entry *prev;			/* Link to the previous entry in the timer group chain */
	uint32_t timeout;					/* Time this entry expires providing the timer group is not the ECM_DB_TIMER_GROUPS_MAX */
	ecm_db_timer_group_t group;				/* The timer group to which this entry belongs, if this is ECM_DB_TIMER_GROUPS_MAX then the timer is not running */
	void *arg;						/* Argument returned in callback */
	ecm_db_timer_group_entry_callback_t fn;			/* Function called when timer expires */
};

/*
 * Connection
 * A connection links two mappings (hence two hosts).  This forms a channel of communication between two hosts.
 */
struct ecm_db_connection_instance;

enum ecm_db_directions {
	ECM_DB_DIRECTION_EGRESS_NAT,			/* LAN->WAN NAT */
	ECM_DB_DIRECTION_INGRESS_NAT,			/* WAN->LAN NAT */
	ECM_DB_DIRECTION_NON_NAT,			/* NET<>NET */
	ECM_DB_DIRECTION_BRIDGED,			/* BRIDGED */
};
typedef enum ecm_db_directions ecm_db_direction_t;

/*
 * Connection listener events
 */
typedef void (*ecm_db_connection_listener_added_callback_t)(void *arg, struct ecm_db_connection_instance *ci);	/* Connection added callback */
typedef void (*ecm_db_connection_listener_removed_callback_t)(void *arg, struct ecm_db_connection_instance *ci);	/* Connection removed callback */

/*
 * Connection creator events
 */
typedef void (*ecm_db_connection_final_callback_t)(void *arg);		/* Finaliser callback */

/*
 * Connection defunct event
 */
typedef bool (*ecm_db_connection_defunct_callback_t)(void *arg, int *accel_mode);	/* Defunct callback */

/*
 * Device Type for IPSec Tunnel devices
 */
#define ECM_ARPHRD_IPSEC_TUNNEL_TYPE 31			/* GGG Should be part of the kernel as a general ARPHRD_XYZ but is not */

/*
 * Interface types
 */
enum ecm_db_iface_types {
	ECM_DB_IFACE_TYPE_ETHERNET = 0,			/* Interface is an ethernet type */
	ECM_DB_IFACE_TYPE_PPPOE,			/* Interface is a PPPoE interface (a specific form of PPP that we recognise in the ECM) */
	ECM_DB_IFACE_TYPE_LAG,				/* Interface is a Link Aggregated interface */
	ECM_DB_IFACE_TYPE_VLAN,				/* Interface is a VLAN interface (802.1Q) */
	ECM_DB_IFACE_TYPE_BRIDGE,			/* Interface is a bridge interface */
	ECM_DB_IFACE_TYPE_LOOPBACK,			/* Interface is a loopback interface */
	ECM_DB_IFACE_TYPE_IPSEC_TUNNEL,			/* Interface is a IPSec tunnel interface */
	ECM_DB_IFACE_TYPE_UNKNOWN,			/* Interface is unknown to the ECM */
	ECM_DB_IFACE_TYPE_SIT,				/* IPv6 in IPv4 tunnel (SIT) interface */
	ECM_DB_IFACE_TYPE_TUNIPIP6,			/* IPIP6 Tunnel (TUNNEL6) interface */
	ECM_DB_IFACE_TYPE_PPPOL2TPV2,			/* Interface is a PPPoL2TPV2 interface (a specific form of PPP that we recognise in the ECM) */
	ECM_DB_IFACE_TYPE_PPTP,				/* Interface is a PPTP interface */
	ECM_DB_IFACE_TYPE_MAP_T,			/* Interface is a MAP-T interface */
	ECM_DB_IFACE_TYPE_GRE_TUN,			/* Interface is a GRE TUN tunnel interface */
	ECM_DB_IFACE_TYPE_GRE_TAP,			/* Interface is a GRE TAP tunnel interface */
	ECM_DB_IFACE_TYPE_RAWIP,			/* Interface is a RAWIP interface */
	ECM_DB_IFACE_TYPE_OVPN,				/* Interface is a OVPN interface */
	ECM_DB_IFACE_TYPE_VXLAN,			/* Interface is a VxLAN interface */
	ECM_DB_IFACE_TYPE_OVS_BRIDGE,			/* Interface is a OpenvSwitch bridge interface */
	ECM_DB_IFACE_TYPE_MACVLAN,			/* Interface is a MACVLAN interface */
	ECM_DB_IFACE_TYPE_COUNT,			/* Number of interface types */
};
typedef enum ecm_db_iface_types ecm_db_iface_type_t;

/*
 * Interface information as stored in the ECM db for known types of interface
 */
struct ecm_db_interface_info_ethernet {			/* type == ECM_DB_IFACE_TYPE_ETHERNET */
	uint8_t address[ETH_ALEN];			/* MAC Address of this Interface */
};

#ifdef ECM_INTERFACE_VXLAN_ENABLE
struct ecm_db_interface_info_vxlan {			/* type == ECM_DB_IFACE_TYPE_VXLAN */
	uint32_t vni;					/* VxLAN network identifier */
	uint32_t if_type;				/* VxLAN interface type */
};
#endif

#ifdef ECM_INTERFACE_VLAN_ENABLE
struct ecm_db_interface_info_vlan {			/* type == ECM_DB_IFACE_TYPE_VLAN */
	uint8_t address[ETH_ALEN];			/* MAC Address of this Interface */
	uint16_t vlan_tpid;				/* VLAN tag protocol id */
	uint16_t vlan_tag;				/* VLAN tag of this interface */
};
#endif

#ifdef ECM_INTERFACE_MACVLAN_ENABLE
struct ecm_db_interface_info_macvlan {			/* type == ECM_DB_IFACE_TYPE_MACVLAN */
	uint8_t address[ETH_ALEN];			/* MAC Address of this Interface */
};
#endif

#ifdef ECM_INTERFACE_BOND_ENABLE
struct ecm_db_interface_info_lag {			/* type == ECM_DB_IFACE_TYPE_LAG */
	uint8_t address[ETH_ALEN];			/* MAC Address of this Interface */
};
#endif

struct ecm_db_interface_info_bridge {			/* type == ECM_DB_IFACE_TYPE_BRIDGE */
	uint8_t address[ETH_ALEN];			/* MAC Address of this Interface */
};

#ifdef ECM_INTERFACE_OVS_BRIDGE_ENABLE
struct ecm_db_interface_info_ovs_bridge {		/* type == ECM_DB_IFACE_TYPE_OVS_BRIDGE */
	uint8_t address[ETH_ALEN];			/* MAC Address of this Interface */
};
#endif

#ifdef ECM_INTERFACE_PPPOE_ENABLE
struct ecm_db_interface_info_pppoe {			/* type == ECM_DB_IFACE_TYPE_PPPOE */
	uint16_t pppoe_session_id;			/* PPPoE session ID on this interface, when applicable */
	uint8_t remote_mac[ETH_ALEN];			/* MAC Address of the PPPoE concentrator */
};
#endif
#ifdef ECM_INTERFACE_L2TPV2_ENABLE
struct ecm_db_interface_info_pppol2tpv2 {                  /* type == ECM_DB_IFACE_TYPE_PPPOL2TPV2 */
	struct {
		struct {
			uint32_t tunnel_id;
			uint32_t peer_tunnel_id;
		} tunnel;
		struct {
			uint32_t session_id;
			uint32_t peer_session_id;
		} session;
	} l2tp;

	struct {
		uint16_t sport, dport;
	} udp;

	struct {
		uint32_t  saddr, daddr;
	} ip;
};

#endif

#ifdef ECM_INTERFACE_PPTP_ENABLE
struct ecm_db_interface_info_pptp {
	uint32_t src_ip;
	uint32_t dst_ip;
	uint16_t src_call_id;
	uint16_t dst_call_id;
};
#endif

#ifdef ECM_INTERFACE_MAP_T_ENABLE
struct ecm_db_interface_info_map_t {                  /* type == ECM_DB_IFACE_TYPE_MAP_T */
	int32_t if_index;
};
#endif

#ifdef ECM_INTERFACE_GRE_TUN_ENABLE
struct ecm_db_interface_info_gre_tun {			/* type == ECM_DB_IFACE_TYPE_GRE_TUN */
	int32_t if_index;
	ip_addr_t local_ip;
	ip_addr_t remote_ip;
};
#endif

struct ecm_db_interface_info_unknown {			/* type == ECM_DB_IFACE_TYPE_UNKNOWN */
	uint32_t os_specific_ident;			/* Operating system specific identifier (known only by front end) */
};

struct ecm_db_interface_info_loopback {			/* type == ECM_DB_IFACE_TYPE_LOOPBACK */
	uint32_t os_specific_ident;			/* Operating system specific identifier (known only by front end) */
};

#ifdef ECM_INTERFACE_IPSEC_ENABLE
struct ecm_db_interface_info_ipsec_tunnel {		/* type == ECM_DB_IFACE_TYPE_IPSEC_TUNNEL */
	uint32_t os_specific_ident;			/* Operating system specific identifier (known only by front end) */
	// GGG TODO Flesh this out with tunnel endpoint addressing detal
};
#endif

#ifdef ECM_INTERFACE_SIT_ENABLE
struct ecm_db_interface_info_sit {			/* type == ECM_DB_IFACE_TYPE_SIT */
	ip_addr_t saddr;				/* Tunnel source address */
	ip_addr_t daddr;				/* Tunnel destination addresss */
	uint8_t  tos;					/* Tunnel tos field */
	uint8_t  ttl;					/* Tunnel ttl field */
};
#endif

#ifdef ECM_INTERFACE_TUNIPIP6_ENABLE
struct ecm_db_interface_info_tunipip6 {			/* type == ECM_DB_IFACE_TYPE_TUNIPIP6 */
	ip_addr_t saddr;				/* Tunnel source address */
	ip_addr_t daddr;				/* Tunnel destination address */
	uint32_t flowlabel;				/* Tunnel ipv6 flowlabel */
	uint32_t flags;					/* Tunnel additional flags */
	uint8_t  hop_limit;				/* Tunnel ipv6 hop limit */
};
#endif

#ifdef ECM_INTERFACE_RAWIP_ENABLE
struct ecm_db_interface_info_rawip {			/* type == ECM_DB_IFACE_TYPE_RAWIP */
	uint8_t address[ETH_ALEN];			/* TODO: Random MAC Address generated by kernel */
};
#endif

#ifdef ECM_INTERFACE_OVPN_ENABLE
struct ecm_db_interface_info_ovpn {			/* type == ECM_DB_IFACE_TYPE_OVPN */
	int32_t tun_ifnum;				/* Tunnel interface number */
};
#endif

/*
 * Interface Heirarchy
 * Each connection instance keeps four lists of interfaces.
 * These lists record the 'interface heirarchy' in the from or to direction for the connection for both NAT and non-NAT.
 * For example, a connection that is 'from' a vlan might have two interfaces in its 'from' heirarchy:
 * The raw interface, e.g. eth0 and the vlan interface perhaps eth0.1.
 * NOTE: Commonly referred to as "inner to outermost" interfaces.
 * For a connection the ci->mi->hi->ni->ii records the interfaces that the connection 'sees' for the from/to paths.  I.e. Innermost.
 * But heirarchy lists record the path from the ii to the actual outward facing interface - well, as far as is possible to detect.
 * A heirarchy list is recorded in reverse so in the example here it would list eth0 followed by eth0.1.
 * Therefore the first interface in the list is the outermost interface, which is for acceleration, hopefully an accel engine supported interface.
 * Lists have a finite size.
 */
#define ECM_DB_IFACE_HEIRARCHY_MAX 9 /* This is the number of interfaces allowed in a heirarchy */

#ifdef ECM_MULTICAST_ENABLE
/*
 * Buffer size for multicast destination interface list
 */
#define ECM_DB_TO_MCAST_INTERFACES_SIZE (sizeof(struct ecm_db_iface_instance *) * ECM_DB_MULTICAST_IF_MAX * ECM_DB_IFACE_HEIRARCHY_MAX)

/*
 * ecm_db_multicast_if_heirarchy_get()
 * 	Returns the address of next heirarchy row from base address of the current heirarchy.
 */
static inline struct ecm_db_iface_instance *ecm_db_multicast_if_heirarchy_get(struct ecm_db_iface_instance *heirarchy_base, uint32_t index)
{
	unsigned long *heirarchy_instance = (unsigned long *)heirarchy_base;
	DEBUG_ASSERT(heirarchy_instance, "Bad memory, multicast interfaces list has been already freed\n");
	DEBUG_ASSERT((index <= ECM_DB_MULTICAST_IF_MAX), "Bad index %u\n", index);
	return (struct ecm_db_iface_instance *)(heirarchy_instance + (index * ECM_DB_IFACE_HEIRARCHY_MAX));
}

/*
 * ecm_db_multicast_if_instance_get_at_index()
 *	Returns a single interface instance pointer from heirarchy at position index.
 */
static inline struct ecm_db_iface_instance *ecm_db_multicast_if_instance_get_at_index(struct ecm_db_iface_instance *heirarchy_start, uint32_t index)
{
	unsigned long *iface_instance = (unsigned long *)heirarchy_start;
	DEBUG_ASSERT(iface_instance, "Bad memory, multicast interfaces list has been already freed\n");
	DEBUG_ASSERT((index <= ECM_DB_IFACE_HEIRARCHY_MAX), "Bad first %u\n", index);
	return (struct ecm_db_iface_instance *)(iface_instance + index);
}

/*
 * ecm_db_multicast_if_first_get_at_index()
 *	Returns a index of the first interface in the list at index position.
 */
static inline int32_t *ecm_db_multicast_if_first_get_at_index(int32_t *if_first, uint32_t index)
{
	DEBUG_ASSERT(if_first, "Bad memory, multicast interfaces first has been already freed\n");
	DEBUG_ASSERT((index <= ECM_DB_MULTICAST_IF_MAX), "Bad first %u\n", index);
	return (if_first + index);
}

/*
 * ecm_db_multicast_if_num_get_at_index()
 *	Returns a pointer to 'if_index' entry in an array at index position.
 */
static inline int32_t *ecm_db_multicast_if_num_get_at_index(int32_t *if_num, uint32_t index)
{
	DEBUG_ASSERT(if_num, "Bad memory, ifindex list has been already freed\n");
	return (if_num + index);
}

/*
 * ecm_db_multicast_copy_if_heirarchy()
 * 	Copy a heirarchy into an array of pointers starting from the heirarchy.
 */
static inline void ecm_db_multicast_copy_if_heirarchy(struct ecm_db_iface_instance *if_hr[], struct ecm_db_iface_instance *heirarchy_start)
{
	unsigned long *iface_instance = (unsigned long *)heirarchy_start;
	struct ecm_db_iface_instance **heirarchy;
	int i;

	DEBUG_ASSERT(iface_instance, "Bad memory, multicast interfaces list has been already freed\n");

        for (i = 0; i < ECM_DB_IFACE_HEIRARCHY_MAX; i++) {
		heirarchy = (struct ecm_db_iface_instance **)iface_instance;
                if_hr[i] = *heirarchy;
                iface_instance = iface_instance + 1;
        }
}
#endif
