#ifndef __PAYLOAD_H__
#define __PAYLOAD_H__

/**
  * Data structures describing IKE payloads and headers as found in RFC 5996.
  * 
  * Please note that these structures (fields as well as their data) will be read and written
  * directly to and from the network interface. Therefore memory alignment, endianness and other
  * binary considerations must be taken into account. Thus, you must uphold the following constraints:
  *
  * -> All data structures must begin at 32 bit word limits (be carefull when casting your pointers).
  * -> Numbers must be expressed in network byte order (or the other peer won't understand you)
  */

#include "common_ike.h"
//#include "machine.h"
#include "transforms/auth.h"
#include "uip.h"
#define IKE_MSG_ZERO 0U

/**
  * Payload types as described on p. 74 *
  */
typedef enum {
  IKE_PAYLOAD_NO_NEXT = 0,
  IKE_PAYLOAD_SA = 33,  // Security Association        
  IKE_PAYLOAD_KE,       // Key Exchange                
  IKE_PAYLOAD_IDi,      // Identification - Initiator   
  IKE_PAYLOAD_IDr,      // Identification - Responder  
  IKE_PAYLOAD_CERT,     // Certificate                 
  IKE_PAYLOAD_CERTREQ,  // Certificate Request         
  IKE_PAYLOAD_AUTH,     // Authentication              
  IKE_PAYLOAD_NiNr,     // Nonce (initiator or responder)
  IKE_PAYLOAD_N,        // Notify                      
  IKE_PAYLOAD_D,        // Delete                      
  IKE_PAYLOAD_V,        // Vendor ID                   
  IKE_PAYLOAD_TSi,      // Traffic Selector - Initiator
  IKE_PAYLOAD_TSr,      // Traffic Selector - Responder
  IKE_PAYLOAD_SK,       // Encrypted and Authenticated 
  IKE_PAYLOAD_CP,       // Configuration               
  IKE_PAYLOAD_EAP       // Extensible Authentication   
} ike_payload_type_t;                            



/**
  * The IKEv2 header (p. 70)
  *
                         1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       IKE SA Initiator's SPI                  |
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       IKE SA Responder's SPI                  |
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  Next Payload | MjVer | MnVer | Exchange Type |     Flags     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                          Message ID                           |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                            Length                             |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                     Figure 4:  IKE Header Format
*/
#define IKE_PAYLOADFIELD_IKEHDR_VERSION_STRING 0xblaha
#define IKE_PAYLOADFIELD_IKEHDR_FLAGS_ISORIGALINITIATOR_BM IKE_MSG_GET_8BITMASK(4) // Syntax brokenness?
#define IKE_PAYLOADFIELD_IKEHDR_FLAGS_ISARESPONSE_BM IKE_MSG_GET_8BITMASK(2) // Syntax brokenness?

typedef enum {
  IKE_PAYLOADFIELD_IKEHDR_EXCHTYPE_SA_INIT = 34,
  IKE_PAYLOADFIELD_IKEHDR_EXCHTYPE_IKE_AUTH,
  IKE_PAYLOADFIELD_IKEHDR_EXCHTYPE_CREATE_CHILD_SA,
  IKE_PAYLOADFIELD_IKEHDR_EXCHTYPE_INFORMATIONAL
} ike_payloadfield_ikehdr_exchtype_t;

/**
  * Macros for setting the IKE header. Endian conversions are not performed as they are
  * for all practical purposes unnecessary.
  */
#define SET_IKE_HDR(payload_arg, exchtype, flags, msg_id) \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->version = IKE_PAYLOADFIELD_IKEHDR_VERSION_STRING; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->exchange_type = exchtype; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->flags = flags; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->message_id = msg_id; \
  payload_arg->prior_next_payload = &((ike_payload_ike_hdr_t *) payload_arg->start)->next_payloyad; \
  payload_arg->start += sizeof(ike_payload_ike_hdr_t)


#define SET_IKE_HDR_AS_RESPONDER(ike_hdr, session, next_payload, exchtype) \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->initiator_spi_high = payload_arg->session->peer_spi_high; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->initiator_spi_low = payload_arg->session->peer_spi_low; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->responder_spi_high = IKE_MSG_ZERO; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->responder_spi_low = IKE_STATEM_MYSPI_GET_SPI(payload_arg->session->initiator_and_my_spi); \
  SET_IKE_HDR(payload_arg, exchtype, IKE_PAYLOAD_FLAGS_RESPONDER, payload_arg->session->peer_msg_id)

#define SET_IKE_HDR_AS_INITIATOR(payload_arg, exchtype) \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->responder_spi_high = payload_arg->session->peer_spi_high; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->responder_spi_low = payload_arg->session->peer_spi_low; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->initiator_spi_high = IKE_MSG_ZERO; \
  ((ike_payload_ike_hdr_t *) payload_arg->start)->initiator_spi_low = IKE_STATEM_MYSPI_GET_SPI(payload_arg->session->initiator_and_my_spi); \
  SET_IKE_HDR(payload_arg, exchtype, IKE_PAYLOAD_FLAGS_INITIATOR, payload_arg->session->my_msg_id)


typedef struct {
  // Please note that we treat the IKE SPIs as 4 byte values internally
  uint32_t sa_initiator_spi_high;
  uint32_t sa_initiator_spi_low;
  uint32_t sa_responder_spi_high;
  uint32_t sa_responder_spi_low;
  
  ike_payload_type_t next_payload;
  uint8_t version;
  ike_payloadfield_ikehdr_exchtype_t exchange_type;
  uint8_t flags;
  
  uint32_t message_id;
  uint32_t len; // Length of header + payload
} ike_payload_ike_hdr_t;


/**
  * The generic payload header (p. 73)
  *
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Next Payload  |C|  RESERVED   |         Payload Length        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                     Figure 5:  Generic Payload Header
*/
/**
  * genpayloadhdr is of type *ike_payload_generic_hdr_t
  * payload_arg is of type *payload_arg_t
  * payload_id is of type ike_payload_type_t
  */
#define SET_GENPAYLOADHDR(genpayloadhdr, payload_arg, payload_id) \
                     genpayloadhdr = (ike_payload_generic_hdr_t *) payload_arg->start; \
                     *payload_arg->prior_next_payload = payload_id; \
                     payload_arg->prior_next_payload = &genpayloadhdr->next_payload; \
                     genpayloadhdr->clear = IKE_MSG_ZERO; \
                     payload_arg->start += sizeof(ike_payload_generic_hdr_t)

#define SET_NO_NEXT_PAYLOAD(payload_arg) \
                     *payload_arg->prior_next_payload = IKE_PAYLOAD_NO_NEXT
typedef struct {
  ike_payload_type_t next_payload;
  uint8_t clear;
  uint16_t len; // Length of payload header + payload
} ike_payload_generic_hdr_t;


/**
  * The proposal substructure (p. 78)
  *
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | 0 (last) or 2 |   RESERVED    |         Proposal Length       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    | Proposal Num  |  Protocol ID  |    SPI Size   |Num  Transforms|
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    ~                        SPI (variable)                         ~
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                                                               |
    ~                        <Transforms>                           ~
    |                                                               |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    
          Figure 7:  Proposal Substructure
  */
#define IKE_PAYLOADFIELD_PROPOSAL_LAST 0
#define IKE_PAYLOADFIELD_PROPOSAL_MORE 2
typedef struct {
  uint8_t last_more;
  uint8_t clear;
  uint16_t proposal_len;
  
  uint8_t proposal_number;
  sa_ipsec_proto_type_t proto_id;
  uint8_t spi_size;
  uint8_t numtransforms;
  
  // The SPI field is not included since it is omitted from this
  // payload in the case of IKE negotiation proposal.
} ike_payload_proposal_t;


/**
  * Transform substructure
  *
                     1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | 0 (last) or 3 |   RESERVED    |        Transform Length       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |Transform Type |   RESERVED    |          Transform ID         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                      Transform Attributes                     ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

           Figure 8:  Transform Substructure
  */
#define IKE_PAYLOADFIELD_TRANSFORM_LAST 0
#define IKE_PAYLOADFIELD_TRANSFORM_MORE 3
typedef struct {
  uint8_t last_more;
  uint8_t clear1;
  uint16_t len;

  uint8_t type;
  uint8_t clear2;
  uint16_t id;
} ike_payload_transform_t;


/**
  * Transform attribute (p. 84)
  * The only attribute that the standard defines is that of key length. Therefore
  * AF is always set to 1 and the payload ends after 32 bits.
  *
                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |A|       Attribute Type        |    AF=0  Attribute Length     |
  |F|                             |    AF=1  Attribute Value      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                   AF=0  Attribute Value                       |
  |                   AF=1  Not Transmitted                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

                  Figure 9:  Data Attributes
  */
#define IKE_PAYLOADFIELD_ATTRIB_VAL (((uint16_t) 1) << 15) | UIP_HTONS((uint16_t) SA_ATTRIBUTE_KEYLEN_ID)
typedef struct {
  uint16_t af_attribute_type; // The first bit should always be set
  uint16_t attribute_value;
} ike_payload_attribute_t;


/**
  * Key Exchange payload (p. 87)
  * The field key exchange data is not included the struct since it's of variable length.
  *
                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Next Payload  |C|  RESERVED   |         Payload Length        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   Diffie-Hellman Group Num    |           RESERVED            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                       Key Exchange Data                       ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

            Figure 10:  Key Exchange Payload Format
  */
typedef struct {
  uint16_t dh_group_num;
  uint16_t clear;
} ike_payload_ke_t;


/**
  * Authentication payload (p. 95)
  *
                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Next Payload  |C|  RESERVED   |         Payload Length        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Auth Method   |                RESERVED                       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                      Authentication Data                      ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

             Figure 14:  Authentication Payload Format
  */
//#define SET_AUTHPAYLOAD(authpayload, auth_method) *((uint8_t *) authpayload = (uint32_t) auth_method << 24
typedef struct {
  ike_auth_type_t auth_type;
  uint8_t clear1;
  uint16_t clear2;
} ike_payload_auth_t;


/**
  * Nonce payload (p. 96)

                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Next Payload  |C|  RESERVED   |         Payload Length        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                            Nonce Data                         ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  
               Figure 15:  Nonce Payload Format
  *
  */
// See Section 2.10, p. 44 for a discussion of nonce data length.
// 16 B = 128 bits.
#define IKE_PAYLOAD_MYNONCE_LEN 16
#define IKE_PAYLOAD_PEERNONCE_LEN 32

/**
  * Notify payload (p. 97)
  
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Next Payload  |C|  RESERVED   |         Payload Length        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Protocol ID  |   SPI Size    |      Notify Message Type      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                Security Parameter Index (SPI)                 ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                       Notification Data                       ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

           Figure 16:  Notify Payload Format
  *
  */
typedef struct {
  sa_ipsec_proto_type_t proto_id;
  uint8_t spi_size;
  uint16_t notify_msg_type;
} ike_payload_notify_t;

typedef enum {
  // Error types
  IKE_PAYLOAD_NOTIFY_UNSUPPORTED_CRITICAL_PAYLOAD = 1,
  IKE_PAYLOAD_NOTIFY_INVALID_IKE_SPI = 4,
  IKE_PAYLOAD_NOTIFY_INVALID_MAJOR_VERSION = 5,
  IKE_PAYLOAD_NOTIFY_INVALID_SYNTAX = 7,
  IKE_PAYLOAD_NOTIFY_INVALID_MESSAGE_ID = 9,
  IKE_PAYLOAD_NOTIFY_INVALID_SPI = 11,
  IKE_PAYLOAD_NOTIFY_NO_PROPOSAL_CHOSEN = 14,
  IKE_PAYLOAD_NOTIFY_INVALID_KE_PAYLOAD = 17,
  IKE_PAYLOAD_NOTIFY_AUTHENTICATION_FAILED = 24,
  IKE_PAYLOAD_NOTIFY_SINGLE_PAIR_REQUIRED = 34,
  IKE_PAYLOAD_NOTIFY_NO_ADDITIONAL_SAS = 35,
  IKE_PAYLOAD_NOTIFY_INTERNAL_ADDRESS_FAILURE = 36,
  IKE_PAYLOAD_NOTIFY_FAILED_CP_REQUIRED = 37,
  IKE_PAYLOAD_NOTIFY_TS_UNACCEPTABLE = 38,
  IKE_PAYLOAD_NOTIFY_INVALID_SELECTORS = 39,
  IKE_PAYLOAD_NOTIFY_TEMPORARY_FAILURE = 43,
  IKE_PAYLOAD_NOTIFY_CHILD_SA_NOT_FOUND = 44,
  
  // Informational types
  IKE_PAYLOAD_NOTIFY_INITIAL_CONTACT = 16384,
  IKE_PAYLOAD_NOTIFY_SET_WINDOW_SIZE = 16385,
  IKE_PAYLOAD_NOTIFY_ADDITIONAL_TS_POSSIBLE = 16386,
  IKE_PAYLOAD_NOTIFY_IPCOMP_SUPPORTED = 16387,
  IKE_PAYLOAD_NOTIFY_NAT_DETECTION_SOURCE_IP = 16388,
  IKE_PAYLOAD_NOTIFY_NAT_DETECTION_DESTINATION_IP = 16389,
  IKE_PAYLOAD_NOTIFY_COOKIE = 16390,
  IKE_PAYLOAD_NOTIFY_USE_TRANSPORT_MODE = 16391,
  IKE_PAYLOAD_NOTIFY_HTTP_CERT_LOOKUP_SUPPORTED = 16392,
  IKE_PAYLOAD_NOTIFY_REKEY_SA = 16393,
  IKE_PAYLOAD_NOTIFY_ESP_TFC_PADDING_NOT_SUPPORTED = 16394,
  IKE_PAYLOAD_NOTIFY_NON_FIRST_FRAGMENTS_ALSO = 16395
} notify_msg_type_t;

#define IKE_PAYLOAD_COOKIE_MAX_LEN 64

/**
  * ID payload (p. 87)
  *
                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Next Payload  |C|  RESERVED   |         Payload Length        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   ID Type     |                 RESERVED                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                   Identification Data                         ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

           Figure 11:  Identification Payload Format
  */
typedef enum {
  IKE_ID_IPV4_ADDR = 1,
    // A single four (4) octet IPv4 address.

  IKE_ID_FQDN,
    /*
      A fully-qualified domain name string.  An example of an ID_FQDN
      is "example.com".  The string MUST NOT contain any terminators
      (e.g., NULL, CR, etc.). All characters in the ID_FQDN are ASCII;
      for an "internationalized domain name", the syntax is as defined
      in [IDNA], for example "xn--tmonesimerkki-bfbb.example.net".
    */

   IKE_ID_RFC822_ADDR,
    /*
      A fully-qualified RFC 822 email address string.  An example of a
      ID_RFC822_ADDR is "jsmith@example.com".  The string MUST NOT
      contain any terminators.  Because of [EAI], implementations would
      be wise to treat this field as UTF-8 encoded text, not as
      pure ASCII.
    */
    
   IKE_ID_IPV6_ADDR = 5,
    /*
      A single sixteen (16) octet IPv6 address.
    */
    
   IKE_ID_DER_ASN1_DN = 9,
    /*
      The binary Distinguished Encoding Rules (DER) encoding of an
      ASN.1 X.500 Distinguished Name [PKIX].
    */

   ID_DER_ASN1_GN,
    /*
      The binary DER encoding of an ASN.1 X.509 GeneralName [PKIX].
      */
      
   ID_KEY_ID
    /*
      An opaque octet stream that may be used to pass vendor-
      specific information necessary to do certain proprietary
      types of identification.} id_type_t;
      */
} id_type_t;

#define SET_IDPAYLOAD(id_payload, payload_arg, id) (uint32_t *) \
  id_payload = payload_arg->start; \
  *((uint8_t *) id_payload) = id; \
  payload_arg->start += sizeof(ike_id_payload_t)

typedef struct {
  id_type_t id_type;
  uint8_t clear1;
  uint16_t clear2;
} ike_id_payload_t;


/**
  * Traffic selector (TS) payload (p. 103)
  *
                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Next Payload  |C|  RESERVED   |         Payload Length        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Number of TSs |                 RESERVED                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                       <Traffic Selectors>                     ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

           Figure 19:  Traffic Selectors Payload Format
  */
// The maximum number of TS payloads that we support.
// This value can NOT be changed without altering the algorithm for T
#define IKE_PAYLOADFIELD_MAX_TS_COUNT 2 
#define SET_TSPAYLOAD(ts_payload, number_of_ts) \
           *((uint32_t *) ts_payload) = IKE_MSG_ZERO; \
           ts_payload->number_of_ts = number_of_ts
typedef struct {
  uint8_t number_of_ts;
  uint8_t clear1;
  uint16_t clear2;
} ike_ts_payload_t;


/**
  * Traffic selector (p. 105)
  *
                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   TS Type     |IP Protocol ID*|       Selector Length         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           Start Port*         |           End Port*           |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                         Starting Address*                     ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                                                               |
  ~                         Ending Address*                       ~
  |                                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

              Figure 20: Traffic Selector
  */
// Only IPv6 type selectors are supported
#define IKE_PAYLOADFIELD_TS_NL_ANY_PROTOCOL SPD_SELECTOR_NL_ANY_PROTOCOL
#define IKE_PAYLOADFIELD_TS_IPV6_ADDR_RANGE 8
#define IKE_PAYLOADFIELD_TS_TYPE IKE_PAYLOADFIELD_TS_IPV6_ADDR_RANGE

#define SET_TSSELECTOR_INIT(ts) \
              ts->ts_type = IKE_PAYLOADFIELD_TS_TYPE; \
              ts->selector_len = UIP_HTONS(sizeof(ike_ts_t))

#define SET_TSSAMEADDR(ts, addr) \
              memcpy(&ts->start_addr, addr, sizeof(addr)); \
              memcpy(&ts->end_addr, addr, sizeof(addr))

#define GET_ADDRSETFROMTS(addrset, ts_src, ts_dst) \
              memcpy(&addrset->ip6addr_src_range_from, &ts_src->start_addr, sizeof(uip_ip6addr_t)); \
              memcpy(&addrset->ip6addr_src_range_to, &ts_src->end_addr, sizeof(uip_ip6addr_t)); \
              memcpy(&addrset->ip6addr_dst_range_from, &ts_dst->start_addr, sizeof(uip_ip6addr_t)); \
              memcpy(&addrset->ip6addr_dst_range_to, &ts_dst->end_addr, sizeof(uip_ip6addr_t)); \
              addrset->nextlayer_type = ts_src->proto; \
              addrset->nextlayer_src_port_range_from = ts_src->start_port; \
              addrset->nextlayer_src_port_range_to = ts_src->end_port; \
              addrset->nextlayer_dst_port_range_from = ts_dst->start_port; \
              addrset->nextlayer_dst_port_range_to = ts_dst->end_port

/*
#define SET_TSSELECTOR_ADDR(ts, addr) \
              ts->ts_type = IKE_PAYLOADFIELD_TS_TYPE; \
              ts->proto = addr->nextlayer_type; \
              ts->selector_len = UIP_HTONS(40); \
              ts->start_port = addr->srcport; \
              ts->end_port = addr->srcport; \
              memcpy(&ts->start_addr, addr->srcaddr, sizeof(addr->srcaddr)); \
              memcpy(&ts->end_addr, addr->dstaddr, sizeof(addr->dstaddr))
*/

#define IKE_PAYLOADFIELD_TS_PROTO_ANY 0
typedef struct {
  uip_ip6addr_t start_addr;
  uip_ip6addr_t end_addr;
  uint8_t ts_type;
  uint8_t proto; // nextlayer protocol
  uint16_t selector_len;
  uint16_t start_port;
  uint16_t end_port;
} ike_ts_t;


/**
  * Encrypted (SK) payload (p. 107)
  *
                       1                   2                   3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  | Next Payload  |C|  RESERVED   |         Payload Length        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     Initialization Vector                     |
  |         (length is block size for encryption algorithm)       |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  ~                    Encrypted IKE Payloads                     ~
  +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |               |             Padding (0-255 octets)            |
  +-+-+-+-+-+-+-+-+                               +-+-+-+-+-+-+-+-+
  |                                               |  Pad Length   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  ~                    Integrity Checksum Data                    ~
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

           Figure 21:  Encrypted Payload Format
  */

#endif
