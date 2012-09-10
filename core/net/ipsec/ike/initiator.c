#include <stdlib.h>
#include "sad.h"
#include "common_ike.h"
#include "auth.h"
#include "spd_conf.h"
#include "ecc/ecc.h"
#include "ecc/ecdh.h"

transition_return_t ike_statem_trans_authreq(ike_statem_session_t *session);
state_return_t ike_statem_state_authrespwait(ike_statem_session_t *session);


// Transmit the IKE_SA_INIT message: HDR, SAi1, KEi, Ni
// If cookie_payload in ephemeral_info is non-NULL the first payload in the message will be a COOKIE Notification.
transition_return_t ike_statem_trans_initreq(ike_statem_session_t *session)
{
  payload_arg_t payload_arg = {
    .start = msg_buf,
    .session = session
  };
  
  ike_payload_ike_hdr_t *ike_hdr = (ike_payload_ike_hdr_t *) payload_arg.start;
  SET_IKE_HDR_AS_INITIATOR(&payload_arg, IKE_PAYLOADFIELD_IKEHDR_EXCHTYPE_SA_INIT, IKE_PAYLOADFIELD_IKEHDR_FLAGS_REQUEST);

  return ike_statem_send_sa_init_msg(session, &payload_arg, ike_hdr, (spd_proposal_tuple_t *) CURRENT_IKE_PROPOSAL);
}


/**
  * 
  * INITRESPWAIT --- (AUTHREQ) ---> AUTHRESPWAIT
  *              --- (INITREQ) ---> AUTHRESPWAIT
  */
uint8_t ike_statem_state_initrespwait(ike_statem_session_t *session)
{
  // If everything went well, we should see something like
  // <--  HDR, SAr1, KEr, Nr, [CERTREQ]

  // Otherwise we expect a reply like 
  // COOKIE or INVALID_KE_PAYLOAD  
  
  ike_payload_ike_hdr_t *ike_hdr = (ike_payload_ike_hdr_t *) msg_buf;

  // Store the peer's SPI (in network byte order)
  session->peer_spi_high = ike_hdr->sa_responder_spi_high;
  session->peer_spi_low = ike_hdr->sa_responder_spi_low;
  
  //
  if (ike_statem_parse_sa_init_msg(session, ike_hdr, session->ephemeral_info->ike_proposal_reply) == 0)
    return 0;

  /*-------------- CUT ------------------- */

  /*
  // Store a copy of this first message from the peer for later use
  // in the autentication calculations.
  COPY_FIRST_MSG(session, ike_hdr);
  
  // We process the payloads one by one
  uint8_t *peer_pub_key;
  uint16_t ke_dh_group = 0;  // 0 is NONE according to IANA's IKE registry
  u8_t *ptr = msg_buf + sizeof(ike_payload_ike_hdr_t);
  u8_t *end = msg_buf + uip_datalen();
  ike_payload_type_t payload_type = ike_hdr->next_payload;
  while (ptr < end) { // Payload loop
    const ike_payload_generic_hdr_t *genpayloadhdr = (const ike_payload_generic_hdr_t *) ptr;
    const uint8_t *payload_start = (uint8_t *) genpayloadhdr + sizeof(ike_payload_generic_hdr_t);
    const uint8_t *payload_end = (uint8_t *) genpayloadhdr + uip_ntohs(genpayloadhdr->len);
    ike_payload_ke_t *ke_payload;
    
    PRINTF("Next payload is %d\n", payload_type);
    switch (payload_type) {
    */
      /*
      FIX: Cookies disabled as for now
      case IKE_PAYLOAD_N:
      ike_payload_notify_t *n_payload = (ike_payload_notify_t *) payload_start;
      // Now what starts with the letter C?
      if (n_payload->notify_msg_type == IKE_PAYLOAD_NOTIFY_COOKIE) {
        // C is for cookie, that's good enough for me
      */
        /**
          * Although the RFC doesn't explicitly state that the COOKIE -notification
          * is a solitary payload, I believe the discussion at p. 31 implies this.
          *
          * Re-transmit the IKE_SA_INIT message with the COOKIE notification as the first payload.
          */
      /*
        session->cookie_payload_ptr = genpayloadhdr; // genpayloadhdr points to the cookie data
        IKE_STATEM_TRANSITION(session);
      }
      */
      
    /*
      break;
      
      case IKE_PAYLOAD_SA:
      // We expect this SA offer to a subset of ours

      // Loop over the responder's offer and that of ours in order to verify that the former
      // is indeed a subset of ours.
      if (ike_statem_parse_sa_payload((spd_proposal_tuple_t *) CURRENT_IKE_PROPOSAL, 
                                      (ike_payload_generic_hdr_t *) genpayloadhdr, 
                                      ke_dh_group,
                                      &session->sa,
                                      NULL,
                                      NULL)) {
        PRINTF(IPSEC_IKE "The peer's offer was unacceptable\n");
        return 0;
      }
      
      PRINTF(IPSEC_IKE "Peer proposal accepted\n");
      break;
      
      case IKE_PAYLOAD_NiNr:
      // This is the responder's nonce
      session->ephemeral_info->peernonce_len = payload_end - payload_start;
      memcpy(&session->ephemeral_info->peernonce, payload_start, session->ephemeral_info->peernonce_len);
      PRINTF(IPSEC_IKE "Parsed %u B long nonce from the peer\n", session->ephemeral_info->peernonce_len);
      MEMPRINTF("Peer's nonce", session->ephemeral_info->peernonce, session->ephemeral_info->peernonce_len);
      break;
      
      case IKE_PAYLOAD_KE:
      // This is the responder's public key
      ke_payload = (ike_payload_ke_t *) payload_start;
*/
      /**
        * Our approach to selecting the DH group in the SA proposal is a bit sketchy: We grab the first one that
        * fits with our offer. This will probably work in most cases, but not all:
        
           "The Diffie-Hellman Group Num identifies the Diffie-Hellman group in
           which the Key Exchange Data was computed (see Section 3.3.2).  This
           Diffie-Hellman Group Num MUST match a Diffie-Hellman group specified
           in a proposal in the SA payload that is sent in the same message, and
           SHOULD match the Diffie-Hellman group in the first group in the first
           proposal, if such exists."
                                                                        (p. 87)
                                                                        
          It might be so that the SA payload is positioned after the KE payload, and in that case we will adopt
          the group referred to in the KE payload as the responder's choice for the SA.
          
          (Yes, payloads might be positioned in any order, consider the following from page 30:
          
           "Although new payload types may be added in the future and may appear
           interleaved with the fields defined in this specification,
           implementations SHOULD send the payloads defined in this
           specification in the order shown in the figures in Sections 1 and 2;
           implementations MUST NOT reject as invalid a message with those
           payloads in any other order."
          
          )
        *
        */
/*
      if (session->sa.dh == SA_UNASSIGNED_TYPE) {
        // DH group not assigned because we've not yet processed the SA payload
        // Store a not of this for later SA processing.
        ke_dh_group = uip_ntohs(ke_payload->dh_group_num);
        PRINTF(IPSEC_IKE "KE payload: Using group DH no. %hhu\n", ke_dh_group);
      }
      else {
        // DH group has been assigned since we've already processed the SA
        if (session->sa.dh != uip_ntohs(ke_payload->dh_group_num)) {
          PRINTF(IPSEC_IKE_ERROR "DH group of the accepted proposal doesn't match that of the KE's.\n");
          return 0;
        }
        PRINTF(IPSEC_IKE "KE payload: Using DH group no. %hhu\n", session->sa.dh);
      }
      
      // Store the address to the beginning of the peer's public key
      peer_pub_key = ((uint8_t *) ke_payload) + sizeof(ike_payload_ke_t);
      break;
      
      case IKE_PAYLOAD_N:
      if (ike_statem_handle_notify((ike_payload_notify_t *) payload_start)) {
        ike_statem_remove_session(session);
        return 0;
      }
      break;
      
      case IKE_PAYLOAD_CERTREQ:
      PRINTF(IPSEC_IKE "Ignoring certificate request payload\n");
      break;

      default:
*/
      /**
        * Unknown / unexpected payload. Is the critical flag set?
        *
        * From p. 30:
        *
        * "If the critical flag is set
        * and the payload type is unrecognized, the message MUST be rejected
        * and the response to the IKE request containing that payload MUST
        * include a Notify payload UNSUPPORTED_CRITICAL_PAYLOAD, indicating an
        * unsupported critical payload was included.""
        */
/*
      if (genpayloadhdr->clear) {
        PRINTF(IPSEC_IKE "Error: Encountered an unknown critical payload\n");
        return 0;
      }
      else
        PRINTF(IPSEC_IKE "Ignoring unknown non-critical payload of type %u\n", payload_type);
      // Info: Ignored unknown payload

    } // End payload switch

    ptr = (uint8_t *) payload_end;
    payload_type = genpayloadhdr->next_payload;
  } // End payload loop
  
  if (payload_type != IKE_PAYLOAD_NO_NEXT) {  
    PRINTF(IPSEC_IKE_ERROR "Unexpected end of peer message.\n");
    return 0;
  }
*/
  /**
    * Generate keying material for the IKE SA.
    * See section 2.14 "Generating Keying Material for the IKE SA"
    */
/*
  PRINTF(IPSEC_IKE "Calculating shared Diffie Hellman secret\n");
  ike_statem_get_ike_keymat(session, peer_pub_key);

  session->ephemeral_info->my_child_spi = SAD_GET_NEXT_SAD_LOCAL_SPI;
*/
  /* ----------- CUT --------------- */

  // Jump
  // Transition to state autrespwait
  session->transition_fn = &ike_statem_trans_authreq;
  session->next_state_fn = &ike_statem_state_authrespwait;

  //session->transition_arg = &session_trigger;

  //IKE_STATEM_INCRMYMSGID(session);
  IKE_STATEM_TRANSITION(session);
    
  return 1;

  // This ends the INIT exchange. Borth parties has now negotiated the IKE SA's parameters and created a common DH secret.
  // We will now proceed with the AUTH exchange.
}


// Transmit the IKE_AUTH message:
//    HDR, SK {IDi, [CERT,] [CERTREQ,]
//      [IDr,] AUTH, SAi2, TSi, TSr}
uint16_t ike_statem_trans_authreq(ike_statem_session_t *session) {
  payload_arg_t payload_arg = {
    .start = msg_buf,
    .session = session
  };
  
  // Write the IKE header
  SET_IKE_HDR_AS_INITIATOR(&payload_arg, IKE_PAYLOADFIELD_IKEHDR_EXCHTYPE_IKE_AUTH, IKE_PAYLOADFIELD_IKEHDR_FLAGS_REQUEST);

  return ike_statem_send_auth_msg(session, &payload_arg, session->ephemeral_info->my_child_spi, session->ephemeral_info->spd_entry->offer);
  /*
  // Write a template of the SK payload for later encryption
  ike_payload_generic_hdr_t *sk_genpayloadhdr = (ike_payload_generic_hdr_t *) payload_arg.start;
  ike_statem_prepare_sk(&payload_arg);

  // ID payload. We use the e-mail address type of ID
  ike_payload_generic_hdr_t *id_genpayloadhdr = (ike_payload_generic_hdr_t *) payload_arg.start;
  ike_statem_set_id_payload(&payload_arg, IKE_PAYLOAD_IDi);
  ike_id_payload_t *id_payload = (ike_id_payload_t *) ((uint8_t *) id_genpayloadhdr + sizeof(ike_payload_generic_hdr_t));
  */
  /**
    * Write the AUTH payload (section 2.15)
    *
    * Details depends on the type of AUTH Method specified.
    */
  /*
  ike_payload_generic_hdr_t *auth_genpayloadhdr;
  SET_GENPAYLOADHDR(auth_genpayloadhdr, &payload_arg, IKE_PAYLOAD_AUTH);
  ike_payload_auth_t *auth_payload = (ike_payload_auth_t *) payload_arg.start;
  auth_payload->auth_type = IKE_AUTH_SHARED_KEY_MIC;
  payload_arg.start += sizeof(ike_payload_auth_t);
  
  uint8_t *signed_octets = payload_arg.start + SA_PRF_MAX_OUTPUT_LEN;
  uint16_t signed_octets_len = ike_statem_get_authdata(session, 1, signed_octets, id_payload, uip_ntohs(id_genpayloadhdr->len) - sizeof(ike_payload_generic_hdr_t));
  */
  /**
    * AUTH = prf( prf(Shared Secret, "Key Pad for IKEv2"), <InitiatorSignedOctets>)
    */
  /*
  prf_data_t auth_data = {
    .out = payload_arg.start,
    .data = signed_octets,
    .datalen = signed_octets_len
  };  
  auth_psk(session->sa.prf, &auth_data);
  payload_arg.start += SA_PRF_OUTPUT_LEN(session);
  auth_genpayloadhdr->len = uip_htons(payload_arg.start - (uint8_t *) auth_genpayloadhdr); // Length of the AUTH payload
*/
  /**
    * Write notification requesting the peer to create transport mode SAs
    */
  //ike_statem_write_notification(&payload_arg, SA_PROTO_IKE, 0, IKE_PAYLOAD_NOTIFY_USE_TRANSPORT_MODE, NULL, 0);

  /**
    * Write SAi2 (offer for the child SA)
    */
  //ike_statem_write_sa_payload(&payload_arg, session->ephemeral_info->spd_entry->offer, session->ephemeral_info->my_child_spi);

  /**
    * The TS payload is decided by the triggering packet's header and the policy that applies to it
    *
    * Read more at "2.9.  Traffic Selector Negotiation" p. 40
    */
  //ike_statem_write_tsitsr(&payload_arg);

    
  // Protect the SK payload. Write trailing fields.
  //ike_statem_finalize_sk(&payload_arg, sk_genpayloadhdr, payload_arg.start - (((uint8_t *) sk_genpayloadhdr) + sizeof(ike_payload_generic_hdr_t)));

  //return uip_ntohl(((ike_payload_ike_hdr_t *) msg_buf)->len);  // Return written length
}


/**
  * AUTH response wait state
  */
state_return_t ike_statem_state_authrespwait(ike_statem_session_t *session)
{
  // If everything went well, we should see something like
  // <--  HDR, SK {IDr, [CERT,] AUTH, SAr2, TSi, TSr}

/**/
/*
  ike_payload_ike_hdr_t *ike_hdr = (ike_payload_ike_hdr_t *) msg_buf;
  ike_ts_t *tsi, *tsr;
  ike_id_payload_t *id_data = NULL;
  uint8_t id_datalen;
  ike_payload_auth_t *auth_payload = NULL;
  uint8_t transport_mode_not_accepted = 1;
  
  uint32_t time = clock_time();
  sad_entry_t *outgoing_sad_entry = sad_create_outgoing_entry(time);
  sad_entry_t *incoming_sad_entry = sad_create_incoming_entry(time);

  
  u8_t *ptr = msg_buf + sizeof(ike_payload_ike_hdr_t);
  u8_t *end = msg_buf + uip_datalen();
  ike_payload_type_t payload_type = ike_hdr->next_payload;
  while (ptr < end) { // Payload loop
    const ike_payload_generic_hdr_t *genpayloadhdr = (const ike_payload_generic_hdr_t *) ptr;
    const uint8_t *payload_start = (uint8_t *) genpayloadhdr + sizeof(ike_payload_generic_hdr_t);
    
    PRINTF("Next payload is %hhu, %hu bytes remaining\n", payload_type, uip_datalen() - (ptr - msg_buf));
    switch (payload_type) {
      case IKE_PAYLOAD_SK:
      if ((end -= ike_statem_unpack_sk(session, (ike_payload_generic_hdr_t *) genpayloadhdr)) == 0) {
        PRINTF(IPSEC_IKE_ERROR "SK payload: Integrity check of peer's message failed\n");
        goto fail;
      }
      break;
      
      case IKE_PAYLOAD_N:
      {
        ike_payload_notify_t *notify = (ike_payload_notify_t *) payload_start;
        if (uip_ntohs(notify->notify_msg_type) == IKE_PAYLOAD_NOTIFY_USE_TRANSPORT_MODE)
          transport_mode_not_accepted = 0;
        if (ike_statem_handle_notify(notify))
          goto fail;
      }
      break;
      
      case IKE_PAYLOAD_IDr:
      id_data = (ike_id_payload_t *) payload_start;
      id_datalen = uip_ntohs(genpayloadhdr->len) - sizeof(ike_payload_generic_hdr_t);
      break;
      
      case IKE_PAYLOAD_AUTH:
      MEMPRINTF("auth payload", genpayloadhdr, uip_ntohs(genpayloadhdr->len));
      auth_payload = (ike_payload_auth_t *) ((uint8_t *) genpayloadhdr + sizeof(ike_payload_generic_hdr_t));
      PRINTF("auth_payload: %p\n", auth_payload);
      
      if (auth_payload->auth_type != IKE_AUTH_SHARED_KEY_MIC) {
        PRINTF(IPSEC_IKE_ERROR "Peer using authentication type %hhu instead of pre-shared key authentication\n", auth_payload->auth_type);
        goto fail;
      }
      break;

      case IKE_PAYLOAD_SA:*/
      /**
        * Assert that the responder's child offer is a subset of that of ours
        */
        /*
      if (ike_statem_parse_sa_payload(session->ephemeral_info->spd_entry->offer, 
                                      (ike_payload_generic_hdr_t *) genpayloadhdr,
                                      0,
                                      NULL,
                                      outgoing_sad_entry,
                                      session->ephemeral_info->proposal_reply)) { // We're not using proposal_reply, but the fn. expects it 
        PRINTF(IPSEC_IKE "The peer's offer was unacceptable\n");
        goto fail;
      }
      
      // Set incoming SAD entry
      incoming_sad_entry->spi = session->ephemeral_info->my_child_spi;
      incoming_sad_entry->sa.encr = outgoing_sad_entry->sa.encr;
      incoming_sad_entry->sa.encr_keylen = outgoing_sad_entry->sa.encr_keylen;
      incoming_sad_entry->sa.integ = outgoing_sad_entry->sa.integ;

      PRINTF(IPSEC_IKE "Peer proposal accepted\n");
      break;
      
      case IKE_PAYLOAD_TSi:
      tsi = (ike_ts_t *) (payload_start + sizeof(ike_payload_generic_hdr_t));
      PRINTF("tsi set to: %p\n", tsi);
      break;
      
      case IKE_PAYLOAD_TSr:
      tsr = (ike_ts_t *) (payload_start + sizeof(ike_payload_generic_hdr_t));
      PRINTF("tsr set to: %p\n", tsr);
      break;
      
      default:
    */  /**
        * Unknown / unexpected payload. Is the critical flag set?
        *
        * From p. 30:
        *
        * "If the critical flag is set
        * and the payload type is unrecognized, the message MUST be rejected
        * and the response to the IKE request containing that payload MUST
        * include a Notify payload UNSUPPORTED_CRITICAL_PAYLOAD, indicating an
        * unsupported critical payload was included.""
        */
/*
      if (genpayloadhdr->clear) {
        PRINTF(IPSEC_IKE_ERROR "Encountered an unknown critical payload\n");
        goto fail;
      }
      else
        PRINTF(IPSEC_IKE "Ignoring unknown non-critical payload of type %u\n", payload_type);
      // Info: Ignored unknown payload
    }

    ptr = (uint8_t *) genpayloadhdr + uip_ntohs(genpayloadhdr->len);
    payload_type = genpayloadhdr->next_payload;
  } // End payload loop

  if (payload_type != IKE_PAYLOAD_NO_NEXT) {  
    PRINTF(IPSEC_IKE_ERROR "Unexpected end of peer message.\n");
    goto fail;
  }
  */
  /**
    * Assert that transport mode was accepted
  if (transport_mode_not_accepted) {
    PRINTF(IPSEC_IKE_ERROR "Peer did not accept transport mode child SA\n");
    goto fail;
  }
  */
  
  /**
    * Assert AUTH data
    */
    /*
  if (id_data == NULL || auth_payload == NULL) {
    PRINTF(IPSEC_IKE_ERROR "IDr or AUTH payload is missing\n");
    goto fail;
  }
  {
    uint8_t responder_signed_octets[session->ephemeral_info->peer_first_msg_len + session->ephemeral_info->peernonce_len + SA_PRF_OUTPUT_LEN(session)];
    uint16_t responder_signed_octets_len = ike_statem_get_authdata(session, 0, responder_signed_octets, id_data, id_datalen);
    uint8_t mac[SA_PRF_OUTPUT_LEN(session)];
    */
    /**
      * AUTH = prf( prf(Shared Secret, "Key Pad for IKEv2"), <InitiatorSignedOctets>)
      */
      /*
    prf_data_t auth_data = {
      .out = mac,
      .data = responder_signed_octets,
      .datalen = responder_signed_octets_len
    };  
    auth_psk(session->sa.prf, &auth_data);

    if (memcmp(mac, ((uint8_t *) auth_payload) + sizeof(ike_payload_auth_t), sizeof(mac))) {
      PRINTF(IPSEC_IKE_ERROR "Verification of the peer's AUTH data failed\n");
      goto fail;
    }
    PRINTF(IPSEC_IKE "Peer successfully authenticated\n");
  }*/
  /**
    * Save traffic descriptors
    */
  // Fn: ts_pair_to_addr_set, addr_set_is_a_subset_of_addr_set, (addr_set_to_ts_pair in the future)
  // FIX: Security: Check that the TSs we receive from the peer are a subset of our offer
  /*
  outgoing_sad_entry->traffic_desc.peer_addr_from = outgoing_sad_entry->traffic_desc.peer_addr_to = &outgoing_sad_entry->peer;
  incoming_sad_entry->traffic_desc.peer_addr_from = incoming_sad_entry->traffic_desc.peer_addr_to = &incoming_sad_entry->peer;
  PRINTF("tsi prior to fn: %p\n", tsi);
  
  ts_pair_to_addr_set(&outgoing_sad_entry->traffic_desc, SPD_OUTGOING_TRAFFIC, tsi, tsr);
  ts_pair_to_addr_set(&incoming_sad_entry->traffic_desc, SPD_INCOMING_TRAFFIC, tsi, tsr);
  */
  
  /**
    * Get Child SA keying material as outlined in section 2.17
    *
    *     KEYMAT = prf+(SK_d, Ni | Nr)
    *
    */
  /*
  if (IKE_STATEM_IS_INITIATOR(session))
    ike_statem_get_child_keymat(session, &incoming_sad_entry->sa, &outgoing_sad_entry->sa);
  else
    ike_statem_get_child_keymat(session, &outgoing_sad_entry->sa, &incoming_sad_entry->sa);
  
  PRINTF(IPSEC_IKE "Outgoing SAD entry registered with SPI %u. Incoming SAD entry registered with SPI %u. Encryption type %hhu integrity type %hhu\n",
   outgoing_sad_entry->spi, incoming_sad_entry->spi, incoming_sad_entry->sa.encr, incoming_sad_entry->sa.integ);
  
  PRINTF("OUTGOING SAD ENTRY\n"); 
  PRINTSADENTRY(outgoing_sad_entry);
  PRINTF("INCOMING SAD ENTRY\n"); 
  PRINTSADENTRY(incoming_sad_entry);
  
  // Remove stuff that we don't need
  ike_statem_clean_session(session);

  */

  if (ike_statem_parse_auth_msg(session) == STATE_SUCCESS) {
  
    // Transition to state autrespwait
    session->transition_fn = NULL;
    session->next_state_fn = &ike_statem_state_established_handler;

    //session->transition_arg = &session_trigger;

    //IKE_STATEM_INCRMYMSGID(session);
    return STATE_SUCCESS;
  }

  return STATE_FAILURE;
}
  
  /**
    * Assert values of traffic selectors
    */
  /*
  uint8_t tmp[100];
  ike_statem_write_tsitsr(session, &tmp);
  */
  // Assert Traffic Selectors' syntax
  /*
  ipsec_assert_ts_invariants  
  
  
  ike_statem_assert_tsa_is_subset_of_tsb(ai, ar, tmp + ioffset, tmp + roffset);  
  ts_to_addr_set(ai, ar);
  */
  /**
    * Derive SA key material (KEYMAT calulcation)
    */
  
  /* 
  ike_ts_t *tsi[IKE_PAYLOADFIELD_MAX_TS_COUNT], *tsr[IKE_PAYLOADFIELD_MAX_TS_COUNT];
  ike_statem_write_tsitsr()
  
  // Check that they fit
  //
  
  ike_ts_t *tsi[IKE_PAYLOADFIELD_MAX_TS_COUNT], *tsr[IKE_PAYLOADFIELD_MAX_TS_COUNT];
  tsi[0] = tsi[1] = tsr[0] = tsr[1] = NULL;
  ike_ts_t **tsarr = NULL;

  ipsec_addr_set_t addrset;
  
  // We process the payloads one by one
  long ke_dh_group = -1;
  ptr += msg_buf + sizeof(ike_payload_ike_hdr_t);
  while (ptr - msg_buf < msg_buf_len) { // Payload loop
    ike_payload_generic_hdr_t *genpayloadhdr = ptr;
    uint8_t *payload_start = genpayloadhdr + sizeof(genpayloadhdr);
    uint8_t *payload_end = genpayloadhdr + uip_ntohs(genpayloadhdr->len);
    
    switch (genpayloadhdr->next_type) {

      case IKE_PAYLOAD_AUTH:
      // Verify that the peer's AUTH payload matches its identity
      

      case IKE_PAYLOAD_TSr:
      tsarr = &tsi;
      
      case IKE_PAYLOAD_TSi:
      if (tsarr == NULL)
        tsarr = &tsr;
      
      ike_ts_payload_t *ts_payload = payload_start;

      int i;
      ptr += sizeof(ts_payload);
      for (i = 0; i < ts_payload->number_of_ts; ++i) {
        *tsarr[i] = ptr;
        ptr += sizeof(ike_ts_t);
      }
      
      tsarr = NULL;      
      break;
  }

  
  // Investigate if the TS offer is acceptable
  //
  // The SA that we're trying to set up will transport traffic from the responder (the other party)
  // to us (the initiator). Therefore the responder is the source of the traffic and the initiator is
  // its destination.
  spd_entry_t *spd_entry;
  
  // The goal is to negotiate the largest set of traffic that our policy allows
  //
  // We need to verify that the returned traffic selectors form a subset of those we sent
  
  
  // First attempt: Try to match the wide second traffic selectors
  uint8_t i = IKE_PAYLOADFIELD_MAX_TS_COUNT;
  do {
    if (tsi[i] != NULL && ipsec_assert_ts_invariants(tsi[i]) &&
      tsr[i] != NULL && ipsec_assert_ts_invariants(tsr[i]) &&    
      (spd_entry = spd_get_entry_by_ts(tsi[i], tsr[i], triggering_pkt))->proc_action == SPD_ACTION_PROTECT) {
        goto success;
    }
    --i;
  } while (i >= 0)
  // Error: TSs not acceptable. Kill the session.
  
  success:
  // ok!
  
  addr_set = tsi / tsr
  
  ipsec_addr_set_t traffic;
  traffic
  
  GET_ADDRSETFROMTS(&addrset, ts_src, ts_dst);
}
*/

// 
// Upon receiving a request:
// Create an ipsec_addr_set_t:
//  use TSr2 as the destination
//  use TSi2 as the source
// Try to match it with an SPD entry
// If no match, try the same but for TS*1
// If still no match, send an error
// 
// If match, create the SA using the instanciated SPD entry as the traffic selector
// 




















/**
  * 
  * INITRESPWAIT --- (AUTHREQ) ---> AUTHRESPWAIT
  *              --- (INITREQ) ---> AUTHRESPWAIT
  */
// void ike_statem_state_initrespwait(ike_statem_session_t *session)
// 
// 
// void ike_statem_state_auth_wait(ike_statem_session_t *session)
// {
//   
// }
// 
// 
/**
  * States (nodes) for the session responder machine
  */
//   
// void ike_statem_state_respond_start(ike_statem_session_t *session) // Always given a NULL pointer
// {
//   ike_payload_ikehdr *ikehdr = &msg_buf;
// 
//   // ike_payload_nxt_hdr
// 
//   if (NTOHL(ikehdr.remoteSPI) == 0) {
//     // Init request. Make transition.
//     session = ike_statem_create_new_session( );
//     memcpy(session.remote, uip_addr6_remote, sizeof(uip_addr6_t));
//     session.re
//     session.past = IKE_STATEM_STATE_STARTNETTRAFIC;
//     session.current = IKE_STATEM_STATE_AUTHWAIT;
//   }
// }
// 

/**
  * Transitions (edges)
  */