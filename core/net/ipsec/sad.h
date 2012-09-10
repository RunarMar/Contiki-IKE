#ifndef __SAD_H__
#define __SAD_H__

#include "sa.h"
#include "ipsec.h"
#include "common_ipsec.h"

/**
  * Debug stuff
  */
// Prints the SAD entry located at entry
#define PRINTSADENTRY(entry)                            \
  do {                                                  \
    PRINTADDRSET(&(entry)->traffic_desc);               \
    PRINTF("SPI: %lx\n", (entry)->spi);                  \
    PRINTF("Sequence number: %lu\n", (entry)->seqno);   \
    PRINTF("Window: %lx\n", (entry)->win);               \
    PRINTF("Time of creation: %lu\n", (entry)->time_of_creation);    \
    PRINTF("Bytes transported: %lu\n", (entry)->bytes_transported);  \
    PRINTF("Encr type: %hu\n", (entry)->sa.encr);         \
    PRINTF("Integ type: %hu\n", (entry)->sa.integ);         \
  } while(0)

#define SAD_DYNAMIC_SPI_START 1000
#define SAD_ENTRY_IS_DYNAMIC(sad_entry) (uip_ntohl(sad_entry->spi) >= SAD_DYNAMIC_SPI_START)

#define SAD_REPLAY_ASSERT(sad_entry, new_seqno)     \
  

// For retrieving the next SPI for inbound traffic
#define SAD_GET_NEXT_SAD_LOCAL_SPI next_sad_local_spi++

// No SPI magic value, as implied by the RFC
#define SAD_NO_SPI 0

/**
  * Make a SAD entry ready for use by resetting counters etc
  */  
#define SAD_RESET_ENTRY(entry, seconds)               \
  entry->seqno = 0;                                   \
  entry->time_of_creation = seconds;                  \
  entry->bytes_transported = 0;                       \
  entry->win = 0

// SPD-S cache key
//
// Key for outbound SA:s. Invariant: > 0
// Colation of the following: 
// Last byte of the dst IP address |
// last byte of the destination port | 
// last byte of the source port
// last next header type identifier
//
/*
typedef struct {
  u8_t ip6_lsb_addr
  u8_t layer4_lsb_src_port
  u8_t layer4_lsb_dst_port
  u8_t layer4_type
} sad_spds_key_t;
*/


/**
  * Implementation of the SAD.
  *
  * This implementation also serves as the SPD-S cache.
  * 
  * Standard violations:
  *   * Sequence number can only be 32 bits, never 64 (extended sequence numbers).
  *   * No sequence counter overflow flag. Rollover occurs everytime. (FIX: Confliciting in implementation)
  *   * No anti-replay window
  *   * Nothing related to tunneling: Mode not supported
  *   * No fragment flag: Only required for tunnel mode (which we don't support)
  *   * No bypass DF flag: As only IPv6 is supported, this override for a IPv4-flag is not implemented
  *   * No DSCP fields: Differentiated services are not supported
  *   * No path MTU: This needs to be reviewed...
  *   * The system can not handle multiple SAs using the same selector (traffic_desc). RFC 4301, p. 13: 
        "IPsec implementation MUST permit establishment and maintenance of multiple SAs between a given sender and receiver"
        However, I cannot se any problems with this as Contiki doesn't implement DSCP.
  *
  * INVARIANT: None of the address spaces expressed by the traffic_desc field overlaps that of another.
  * INVARIANT: The field spi is unique for all entries. 
  *
  */
typedef struct x2 {
  struct y *next;
  
  /**
    * Traffic descriptor for the SA entry.
    *
    * This field is used to associate \b outgoing traffic with certain SAs. For example; a packet whose traffic selector is destined
    * for the PROTECT policy in the SPD might be associated with an SA in the SAD. Remember that for every SPD entry there might be
    * several SAs in the SAD due to the PFP mechanism. \c traffic_desc allows us to discriminate what SA to apply to an outgoing
    * packet requiring protection. This makes this table a SPD-S cache implementation as well.
    *
    * In the context of \b incoming traffic it's used to verify that an SA referenced (via the SPI) by the packet has a traffic
    * selector that includes the packet's tag in question.
    *
    * Please note that although the \c traffic_desc can express IPv6 address ranges only one address is used on for each end
    * in sad_entry_t. This is because this implementation only supports transport mode (see section 1.1.2 RFC 5996) unicast.
    *
    */
  ipsec_addr_set_t traffic_desc;
    
  // The author can't see any reason as to why we should store the SPIs in host byte order.
  // Therefore the SPI below is stored in network byte order. This saves some memory by eliding the conversion.
  u32_t spi; // Suggestion for the future: 16 bits for incoming traffic, 32 bits for outoing
  
  // Encryption
  sa_child_t sa;
  
  // Sequence number overflow is always permitted for static  (FIX: Different policies for overflow in different parts of the code)
  /**
    * Replay protection
    *
    * Replay protection is described in the RFC 4301 (IPsec), 4303 (ESP) and 4302 (AH).
    * This implementation offers a 32 packet wide sliding window for all dynamic SAs.
    */
  // Incoming traffic: Highest verified sequence number of this SA. The right edge of the window.  
  // Outgoing traffic: Sequence number of the last transmitted packet
  u32_t seqno;
  
  // Incoming traffic: 32 positions long window mask
  // Outgoing traffic: Unused
  u32_t win;
      
  /**
    * Timestamp indicating the time of creation of the SA. It also serves the purpose of distinguishing between manual SAs
    * (created by an administrator) and automatic ones (created by IKE). The former has a value of zero, while the value
    * of the latter is the time of its creation (and thus non-zero).
    *
    * Manual SAs does not enjoy anti-replay protection as it's too much to ask from an administrator to keep the sequence numbers synchronized
    * between the hosts across reboots etc. This is not a problem in the automatic case though as the SAs are synchronized upon creation
    * and discarded at reboot.
    */
  u32_t time_of_creation;

  // The number of bytes transported over the SA
  u32_t bytes_transported;
} sad_entry_t;



/**
  * IP header to SPDS Key
  *
  */
void sad_init(void);
u8_t sad_incoming_replay(sad_entry_t *entry, u32_t seqno);
sad_entry_t *sad_get_outgoing(ipsec_addr_t *outgoing_pkt);
sad_entry_t *sad_get_incoming(u32_t spi);
sad_entry_t *sad_create_incoming_entry(u32_t time_of_creation);
sad_entry_t *sad_create_outgoing_entry(u32_t time_of_creation);
void sad_remove_outgoing_entry(sad_entry_t *sad_entry);
void sad_remove_incoming_entry(sad_entry_t *sad_entry);
void sad_conf();

#endif

