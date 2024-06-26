/**
 * @file    evm_txn_helpers.c
 * @author  Cypherock X1 Team
 * @brief   Helper implementation for interpreting and signing EVM
 *          transactions
 * @copyright Copyright (c) 2023 HODL TECH PTE LTD
 * <br/> You may obtain a copy of license at <a href="https://mitcc.org/"
 *target=_blank>https://mitcc.org/</a>
 *
 ******************************************************************************
 * @attention
 *
 * (c) Copyright 2023 by HODL TECH PTE LTD
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

#include "evm_txn_helpers.h"

#include "eip1159.h"
#include "evm_helpers.h"
#include "evm_priv.h"
#include "int-util.h"

/*****************************************************************************
 * EXTERN VARIABLES
 *****************************************************************************/

/*****************************************************************************
 * PRIVATE MACROS AND DEFINES
 *****************************************************************************/

/*****************************************************************************
 * PRIVATE TYPEDEFS
 *****************************************************************************/

/*****************************************************************************
 * STATIC FUNCTION PROTOTYPES
 *****************************************************************************/

/**
 * @brief Decodes transaction buffer based on the structure defined in EIP-155
 * @details The function ensures that entire buffer is processed. If buffer
 * contains extra data (which is left unprocessed after decoding all the rlp
 * elements defined in the EIP), it will return false indicating failure in
 * decoding. Refer: https://eips.ethereum.org/EIPS/eip-155
 *
 * @param data          The buffer containing encoded ethereum transaction as
 * defined in EIP-155
 * @param data_size     The size of the data buffer
 * @param txn_context   Pointer to an instance of evm_txn_context_t which will
 * hold the decoded fields
 *
 * @return bool Indicating if parsing succeeded
 * @retval true If transaction is parsed successfully
 * @retval false If transaction parsing fails
 */
static bool evm_parse_legacy(const uint8_t *data,
                             size_t data_size,
                             evm_txn_context_t *txn_context);

/**
 * @brief
 *
 * @param txn_context
 * @return EVM_TRANSACTION_TYPE
 */
static EVM_TRANSACTION_TYPE evm_decode_transaction_type(
    evm_txn_context_t *txn_context);

/*****************************************************************************
 * STATIC VARIABLES
 *****************************************************************************/

/*****************************************************************************
 * GLOBAL VARIABLES
 *****************************************************************************/

/*****************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

static bool evm_parse_legacy(const uint8_t *data,
                             size_t data_size,
                             evm_txn_context_t *txn_context) {
  if (data == NULL || txn_context == NULL) {
    return false;
  }
  evm_unsigned_txn *utxn_ptr = &txn_context->transaction_info;
  memzero(utxn_ptr, sizeof(evm_unsigned_txn));

  seq_type type = NONE;
  int64_t offset = 0;
  uint64_t decoded_len = 0;
  uint64_t item_bytes_len = 0;

  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len;
  if (type != LIST)
    return false;

  // nonce
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len;
  if (type != STRING)
    return false;
  utxn_ptr->nonce_size[0] = CY_MAX(1, item_bytes_len);
  s_memcpy(utxn_ptr->nonce, data, data_size, item_bytes_len, &offset);

  // gas price
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len;
  if (type != STRING)
    return false;
  utxn_ptr->gas_price_size[0] = CY_MAX(1, item_bytes_len);
  s_memcpy(utxn_ptr->gas_price, data, data_size, item_bytes_len, &offset);

  // gas limit
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len;
  if (type != STRING)
    return false;
  utxn_ptr->gas_limit_size[0] = CY_MAX(1, item_bytes_len);
  s_memcpy(utxn_ptr->gas_limit, data, data_size, item_bytes_len, &offset);

  // to address
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len;
  if (type != STRING)
    return false;
  s_memcpy(utxn_ptr->to_address, data, data_size, item_bytes_len, &offset);

  // value
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len;
  if (type != STRING)
    return false;
  utxn_ptr->value_size[0] = CY_MAX(1, item_bytes_len);
  s_memcpy(utxn_ptr->value, data, data_size, item_bytes_len, &offset);

  // data
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len;
  if (type != STRING)
    return false;
  utxn_ptr->data_size = item_bytes_len;
  utxn_ptr->data = &data[offset];
  offset += (int64_t)item_bytes_len;

  // chain id
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len;
  if (type != STRING)
    return false;
  utxn_ptr->chain_id_size[0] = CY_MAX(1, item_bytes_len);
  s_memcpy(utxn_ptr->chain_id, data, data_size, item_bytes_len, &offset);

  // r: Should be dummy (i.e. 0); no storage needed
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len + item_bytes_len;
  if (type != STRING)
    return false;

  // s: Should be dummy (i.e. 0); no storage needed
  item_bytes_len =
      get_decode_length(data + offset, data_size - offset, &decoded_len, &type);
  offset += decoded_len + item_bytes_len;
  if (type != STRING)
    return false;

  return (offset == data_size);
}

static EVM_TRANSACTION_TYPE evm_decode_transaction_type(
    evm_txn_context_t *txn_context) {
  if (0 == txn_context->transaction_info.data_size) {
    return EVM_TXN_NO_DATA;
  }

  if (EVM_FUNC_SIGNATURE_LENGTH > txn_context->transaction_info.data_size) {
    // function signature should be of 4-bytes
    return EVM_TXN_INVALID_DATA;
  }

  uint32_t function_tag = U32_READ_BE_ARRAY(txn_context->transaction_info.data);
  if (EVM_transfer_TAG == function_tag &&
      g_evm_app->is_token_whitelisted(txn_context->transaction_info.to_address,
                                      &txn_context->contract)) {
    return EVM_TXN_TOKEN_TRANSFER_FUNC;
  }

  if (EVM_swap_TAG == function_tag || EVM_uniswapV3Swap_TAG == function_tag ||
      EVM_safeTransferFrom_TAG == function_tag ||
      EVM_deposit_TAG == function_tag || EVM_transfer_TAG == function_tag) {
    // decode the contract data for display
    if (ETH_UTXN_ABI_DECODE_OK !=
        ETH_ExtractArguments(txn_context->transaction_info.data,
                             txn_context->transaction_info.data_size,
                             &txn_context->display_node)) {
      /**
       * this could mean that the parameters to the function (provided in the
       * transaction.data) are invalid. This means either of the following: the
       * argument count mismatch, arguments are in the wrong order, or the
       * arguments are of wrong type
       */
      return EVM_TXN_INVALID_DATA;
    } else {
      return EVM_TXN_KNOWN_FUNC_SIG;
    }
  }

  // unidentified function signature
  return EVM_TXN_UNKNOWN_FUNC_SIG;
}

/*****************************************************************************
 * GLOBAL FUNCTIONS
 *****************************************************************************/

bool evm_decode_unsigned_txn(const uint8_t *data,
                             size_t data_size,
                             evm_txn_context_t *txn_context) {
  bool status = false;
  uint8_t txn_version = data[0] > 0x7f ? 0 : data[0];
  switch (txn_version) {
    case 0:
      status = evm_parse_legacy(data, data_size, txn_context);
      break;

    case 1:
      status = evm_parse_eip2930(&data[1], data_size - 1, txn_context);
      break;

    case 2:
      status = evm_parse_eip1559(&data[1], data_size - 1, txn_context);
      break;

    default:
      // Error: Unknown transaction type
      status = false;
      break;
  }

  if (true == status) {
    txn_context->txn_type = evm_decode_transaction_type(txn_context);
  }
  return status;
}

bool evm_validate_unsigned_txn(const evm_txn_context_t *txn_context) {
  const evm_unsigned_txn *utxn_ptr = &txn_context->transaction_info;
  return !(
      // Check if the chain id size or nonce size is non-zero
      (utxn_ptr->chain_id_size[0] == 0 || utxn_ptr->nonce_size[0] == 0) ||
      // Check if the gas limit is non-zero
      (is_zero(utxn_ptr->gas_limit, utxn_ptr->gas_limit_size[0])) ||
      // Check if the gas price is non-zero
      (is_zero(utxn_ptr->gas_price, utxn_ptr->gas_price_size[0])) ||
      // Check if the chain id from app matches with the chain id from the
      // unsigned transaction
      (cy_read_be(utxn_ptr->chain_id, utxn_ptr->chain_id_size[0]) !=
       g_evm_app->chain_id) ||
      // ensure token transfer is triggered with zero ETH amount
      (EVM_TXN_TOKEN_TRANSFER_FUNC == txn_context->txn_type &&
       !is_zero(utxn_ptr->value, utxn_ptr->value_size[0])) ||
      // Check if token transfer is triggered with whitelisted token; ensure
      // reference is valid
      (EVM_TXN_TOKEN_TRANSFER_FUNC == txn_context->txn_type &&
       NULL == txn_context->contract) ||
      // ensure the payload status is valid
      (txn_context->txn_type == EVM_TXN_INVALID_DATA));
}

void eth_get_to_address(const evm_txn_context_t *txn_context,
                        const uint8_t **address) {
  const uint8_t *addr = NULL;
  if (EVM_TXN_TOKEN_TRANSFER_FUNC == txn_context->txn_type) {
    addr = &txn_context->transaction_info.data[16];
  } else {
    addr = &txn_context->transaction_info.to_address[0];
  }

  if (NULL != address) {
    *address = addr;
  }
}

uint32_t eth_get_value(const evm_txn_context_t *txn_context, char *value) {
  const evm_unsigned_txn *utxn_ptr = &txn_context->transaction_info;
  if (EVM_TXN_TOKEN_TRANSFER_FUNC == txn_context->txn_type) {
    byte_array_to_hex_string(utxn_ptr->data + EVM_FUNC_SIGNATURE_LENGTH +
                                 EVM_FUNC_PARAM_BLOCK_LENGTH,
                             EVM_FUNC_PARAM_BLOCK_LENGTH,
                             value,
                             2 * EVM_FUNC_PARAM_BLOCK_LENGTH + 1);
    return 2 * EVM_FUNC_PARAM_BLOCK_LENGTH;
  } else {
    byte_array_to_hex_string(utxn_ptr->value,
                             utxn_ptr->value_size[0],
                             value,
                             2 * utxn_ptr->value_size[0] + 1);
    return 2 * utxn_ptr->value_size[0];
  }
}

void eth_get_fee_string(const evm_unsigned_txn *utxn_ptr,
                        char *fee_decimal_string,
                        uint8_t size,
                        uint8_t decimal) {
  uint8_t fee[16] = {0};
  uint64_t txn_fee, carry;
  char fee_hex_string[33] = {'\0'};

  // make sure we do not process over the current capacity (i.e., 8-byte limit
  // for gas limit and price each)
  ASSERT(utxn_ptr->gas_price_size[0] <= 8 && utxn_ptr->gas_limit_size[0] <= 8);
  // Capacity to multiply 2 numbers upto 8-byte value and store the result in 2
  // separate 8-byte variables
  txn_fee = mul128(
      bendian_byte_to_dec(utxn_ptr->gas_price, utxn_ptr->gas_price_size[0]),
      bendian_byte_to_dec(utxn_ptr->gas_limit, utxn_ptr->gas_limit_size[0]),
      &carry);
  // prepare the whole 128-bit little-endian representation of fee
  memcpy(fee, &txn_fee, sizeof(txn_fee));
  memcpy(fee + sizeof(txn_fee), &carry, sizeof(carry));
  // outputs 128-bit (16-byte) big-endian representation of fee
  cy_reverse_byte_array(fee, sizeof(fee));
  byte_array_to_hex_string(
      fee, sizeof(fee), fee_hex_string, sizeof(fee_hex_string));
  convert_byte_array_to_decimal_string(sizeof(fee_hex_string) - 1,
                                       decimal,
                                       fee_hex_string,
                                       fee_decimal_string,
                                       size);
}

uint8_t evm_get_decimal(const evm_txn_context_t *txn_context) {
  if (EVM_TXN_TOKEN_TRANSFER_FUNC == txn_context->txn_type) {
    return txn_context->contract->decimal;
  }
  return ETH_DECIMAL;
}

const char *evm_get_asset_symbol(const evm_txn_context_t *txn_context) {
  if (EVM_TXN_TOKEN_TRANSFER_FUNC == txn_context->txn_type) {
    return txn_context->contract->symbol;
  }
  return g_evm_app->lunit_name;
}
