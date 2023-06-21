/**
 * @file    pair_card_controller.c
 * @author  Cypherock X1 Team
 * @brief   Pair card controller.
 *          This file contains the pair card controller and helper functions.
 * @copyright Copyright (c) 2022 HODL TECH PTE LTD
 * <br/> You may obtain a copy of license at <a href="https://mitcc.org/"
 *target=_blank>https://mitcc.org/</a>
 *
 ******************************************************************************
 * @attention
 *
 * (c) Copyright 2022 by HODL TECH PTE LTD
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * "Commons Clause" License Condition v1.0
 *
 * The Software is provided to you by the Licensor under the License,
 * as defined below, subject to the following condition.
 *
 * Without limiting other conditions in the License, the grant of
 * rights under the License will not include, and the License does not
 * grant to you, the right to Sell the Software.
 *
 * For purposes of the foregoing, "Sell" means practicing any or all
 * of the rights granted to you under the License to provide to third
 * parties, for a fee or other consideration (including without
 * limitation fees for hosting or consulting/ support services related
 * to the Software), a product or service whose value derives, entirely
 * or substantially, from the functionality of the Software. Any license
 * notice or attribution required by the License must also include
 * this Commons Clause License Condition notice.
 *
 * Software: All X1Wallet associated files.
 * License: MIT
 * Licensor: HODL TECH PTE LTD
 *
 ******************************************************************************
 */

/*****************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "app_error.h"
#include "atca_status.h"
#include "base58.h"
#include "bip32.h"
#include "card_internal.h"
#include "card_utils.h"
#include "controller_level_four.h"
#include "core_error.h"
#include "curves.h"
#include "nist256p1.h"
#include "ui_instruction.h"
#include "utils.h"

/*****************************************************************************
 * EXTERN VARIABLES
 *****************************************************************************/

/*****************************************************************************
 * PRIVATE MACROS AND DEFINES
 *****************************************************************************/

/*****************************************************************************
 * PRIVATE TYPEDEFS
 *****************************************************************************/
typedef struct card_pairing_data {
  bool is_paired;
  uint8_t data[128];
  uint8_t data_len;
  uint8_t session_nonce[32];
} card_pairing_data_t;

/*****************************************************************************
 * STATIC FUNCTION PROTOTYPES
 *****************************************************************************/

/*****************************************************************************
 * STATIC VARIABLES
 *****************************************************************************/

/**
 * This function performs pre-processing for a pairing operation, including
 * generating a session nonce, creating a data packet, and signing the data with
 * a private key.
 *
 * @param pair_data A pointer to a struct containing data related to the card
 * pairing process.
 *
 * @return a uint32_t value, which could be a success code or an error code.
 */
static uint32_t pair_card_preprocess(card_pairing_data_t *pair_data);

/**
 * This function processes a card pairing request by verifying the signature,
 * performing an ECDH key exchange, and setting the pairing key in the keystore.
 *
 * @param pair_data A pointer to a struct containing data related to the card
 * pairing process, including session nonce, data exchanged during pairing, and
 * signatures.
 * @param card_number The number of the card being paired.
 *
 * @return a uint32_t value, which could be either a success code or an error
 * code.
 */
static uint32_t pair_card_postprocess(card_pairing_data_t *pair_data,
                                      uint8_t card_number);

/**
 * The function handles pairing success by setting the family ID and performing
 * post-processing if the card is not already paired.
 *
 * @param card_data A pointer to a struct containing data related to the card
 * operation.
 * @param pair_data A pointer to a struct containing data related to the pairing
 * process of a card.
 *
 * @return a uint32_t value, which could be an error status or the status of the
 * NFC data.
 */
static uint32_t handle_pairing_success(card_operation_data_t *card_data,
                                       card_pairing_data_t *pair_data);

/**
 * The function initializes a card applet and checks if the card is paired, and
 * if not, proceeds with pairing.
 *
 * @param card_data A pointer to a struct containing data related to the NFC
 * card operation, such as the card's family ID, card key ID, and error type.
 * @param pair_data A pointer to a struct containing data related to card
 * pairing, including whether the card is already paired and the data needed for
 * pairing.
 *
 * @return void, which means it does not return any value.
 */
static void init_and_pair_card(card_operation_data_t *card_data,
                               card_pairing_data_t *pair_data);

/*****************************************************************************
 * GLOBAL VARIABLES
 *****************************************************************************/

/*****************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/
static uint32_t pair_card_preprocess(card_pairing_data_t *pair_data) {
  uint8_t digest[64] = {0}, sig[65] = {0};
  uint8_t invalid_self_keypath[8] = {DEFAULT_VALUE_IN_FLASH,
                                     DEFAULT_VALUE_IN_FLASH,
                                     DEFAULT_VALUE_IN_FLASH,
                                     DEFAULT_VALUE_IN_FLASH,
                                     DEFAULT_VALUE_IN_FLASH,
                                     DEFAULT_VALUE_IN_FLASH,
                                     DEFAULT_VALUE_IN_FLASH,
                                     DEFAULT_VALUE_IN_FLASH};

  pair_data->data_len = 44;

  /// Pair operation pre-processing
  random_generate(pair_data->session_nonce, sizeof(pair_data->session_nonce));
  memcpy(pair_data->data, get_perm_self_key_id(), 4);
  memcpy(pair_data->data + 4,
         pair_data->session_nonce,
         sizeof(pair_data->session_nonce));
  memcpy(pair_data->data + 36, get_perm_self_key_path(), 8);

  if (memcmp(get_perm_self_key_path(),
             invalid_self_keypath,
             sizeof(invalid_self_keypath)) == 0) {
    /* Device is not provisioned */
    mark_core_error_screen(ui_text_device_compromised);
    return EXCEPTION_INVALID_PROVISION_DATA;
  }

  /// Sign pairing data and append signature
  sha256_Raw(pair_data->data, pair_data->data_len, digest);
  uint8_t status = atecc_nfc_sign_hash(digest, sig);
  if (ATCA_SUCCESS != status) {
    LOG_CRITICAL("xxec %d:%d", ATECC_ERROR_BASE + status, __LINE__);
    return ATECC_ERROR_BASE + status;
  }

  pair_data->data_len +=
      ecdsa_sig_to_der(sig, pair_data->data + pair_data->data_len);

  pair_data->is_paired = false;
  return SUCCESS;
}

static uint32_t pair_card_postprocess(card_pairing_data_t *pair_data,
                                      uint8_t card_number) {
  HDNode guest_card_node;
  SHA512_CTX ctx;
  uint8_t keystore_index, digest[32] = {0}, buffer[64] = {0};
  uint8_t public_key_uncompressed[65] = {0};
  uint32_t index;
  char xpub[112] = {'\0'};

  base58_encode_check(get_card_root_xpub(),
                      FS_KEYSTORE_XPUB_LEN,
                      nist256p1_info.hasher_base58,
                      xpub,
                      112);
  hdnode_deserialize_public(
      (char *)xpub, 0x0488b21e, NIST256P1_NAME, &guest_card_node, NULL);

  index = read_be(pair_data->data + 36);
  hdnode_public_ckd(&guest_card_node, index);

  index = read_be(pair_data->data + 40);
  hdnode_public_ckd(&guest_card_node, index);

  der_to_sig(pair_data->data + 44, buffer);
  sha256_Raw(pair_data->data, 44, digest);
  uint8_t status = ecdsa_verify_digest(
      &nist256p1, guest_card_node.public_key, buffer, digest);
  if (status) {
    LOG_CRITICAL("xxec %d:%d", EXCEPTION_CARD_NOT_VERIFIED + status, __LINE__);
    log_hex_array("resp", pair_data->data, 128);
    log_hex_array("sig", buffer, sizeof(buffer));
    mark_core_error_screen(ui_text_cannot_verify_card_contact_support);
    return EXCEPTION_CARD_NOT_VERIFIED + status;
  }

  keystore_index = card_number - 1;
  ecdsa_uncompress_pubkey(
      &nist256p1, guest_card_node.public_key, public_key_uncompressed);
  status = atecc_nfc_ecdh(&public_key_uncompressed[1], pair_data->data + 45);
  if (ATCA_SUCCESS != status) {
    LOG_CRITICAL("xxec %d:%d", ATECC_ERROR_BASE + status, __LINE__);
    return ATECC_ERROR_BASE + status;
  }

  sha512_Init(&ctx);
  sha512_Update(&ctx, pair_data->data + 45, 32);
  sha512_Update(&ctx, pair_data->data + 4, 32);
  sha512_Update(&ctx, pair_data->session_nonce, 32);
  sha512_Final(&ctx, buffer);
  set_keystore_pairing_key(
      keystore_index, buffer, sizeof(buffer), FLASH_SAVE_LATER);
  set_keystore_key_id(keystore_index, pair_data->data, 4, FLASH_SAVE_LATER);
  set_keystore_used_status(keystore_index, 1, FLASH_SAVE_NOW);

  return SUCCESS;
}

static uint32_t handle_pairing_success(card_operation_data_t *card_data,
                                       card_pairing_data_t *pair_data) {
  if (false == pair_data->is_paired) {
    if (*(uint32_t *)get_family_id() == DEFAULT_UINT32_IN_FLASH) {
      set_family_id_flash(card_data->nfc_data.family_id);
    }

    uint32_t error_status =
        pair_card_postprocess(pair_data, card_data->nfc_data.tapped_card);
    if (SUCCESS != error_status) {
      return error_status;
    }
  }
  return card_data->nfc_data.status;
}

static void init_and_pair_card(card_operation_data_t *card_data,
                               card_pairing_data_t *pair_data) {
  ASSERT(NULL != card_data && NULL != pair_data);

  memcpy(card_data->nfc_data.family_id, get_family_id(), FAMILY_ID_SIZE);

  // Initialize card applet
  card_initialize_applet(card_data);

  if (CARD_OPERATION_SUCCESS == card_data->error_type) {
    // if card tapped is not paired, proceed with pairing
    pair_data->is_paired =
        (-1 != get_paired_card_index(card_data->nfc_data.card_key_id)) ? true
                                                                       : false;
    if (false == pair_data->is_paired) {
      card_data->nfc_data.status =
          nfc_pair(pair_data->data, &(pair_data->data_len));

      if (card_data->nfc_data.status != SW_NO_ERROR) {
        card_handle_errors(card_data);
      }
    }
  }
  return;
}

/*****************************************************************************
 * GLOBAL FUNCTIONS
 *****************************************************************************/
uint32_t card_pair_without_retap(uint8_t card_number) {
  ASSERT(1 <= card_number && 4 >= card_number);

  uint32_t error_status = DEFAULT_UINT32_IN_FLASH;
  card_operation_data_t card_data = {
      .error_message = NULL,
      .error_type = CARD_OPERATION_DEFAULT_INVALID,
      .nfc_data = {0}};
  card_pairing_data_t pair_data = {0};

  error_status = pair_card_preprocess(&pair_data);
  if (SUCCESS != error_status) {
    return error_status;
  }

  while (1) {
    // Initialize card tap config
    card_data.nfc_data.acceptable_cards = encode_card_number(card_number);

    init_and_pair_card(&card_data, &pair_data);

    if (CARD_OPERATION_SUCCESS == card_data.error_type) {
      error_status = handle_pairing_success(&card_data, &pair_data);
      buzzer_start(BUZZER_DURATION);
      break;
    }

    if (CARD_OPERATION_CARD_REMOVED != card_data.error_type) {
      break;
    }
  }

  nfc_deselect_card();
  return error_status;
}

uint32_t card_pair_operation(uint8_t card_number,
                             char *heading,
                             char *message) {
  ASSERT(1 <= card_number && 4 >= card_number && NULL != message);

  uint32_t error_status = DEFAULT_UINT32_IN_FLASH;
  card_operation_data_t card_data = {
      .error_message = NULL,
      .error_type = CARD_OPERATION_DEFAULT_INVALID,
      .nfc_data = {0}};
  card_pairing_data_t pair_data = {0};

  error_status = pair_card_preprocess(&pair_data);
  if (SUCCESS != error_status) {
    return error_status;
  }

  instruction_scr_init(message, heading);
  card_data.nfc_data.retries = 5;

  while (1) {
    // Initialize card tap config
    card_data.nfc_data.acceptable_cards = encode_card_number(card_number);

    init_and_pair_card(&card_data, &pair_data);

    if (CARD_OPERATION_SUCCESS == card_data.error_type) {
      buzzer_start(BUZZER_DURATION);
      error_status = handle_pairing_success(&card_data, &pair_data);
      wait_for_card_removal();
      break;
    }

    if ((CARD_OPERATION_CARD_REMOVED == card_data.error_type) ||
        (CARD_OPERATION_RETAP_BY_USER_REQUIRED == card_data.error_type)) {
      const char *error_msg = card_data.error_message;
      if (CARD_OPERATION_SUCCESS == indicate_card_error(error_msg)) {
        // Re-render the instruction screen
        instruction_scr_init(message, heading);
        continue;
      }
    } else {
      error_status = card_data.nfc_data.status;
      break;
    }
  }

  nfc_deselect_card();
  return error_status;
}