/**
 * @file    solana.h
 * @author  Cypherock X1 Team
 * @brief   Header for solana coin.
 *          Stores declarations for solana coin functions.
 * @copyright Copyright (c) 2022 HODL TECH PTE LTD
 * <br/> You may obtain a copy of license at <a href="https://mitcc.org/" target=_blank>https://mitcc.org/</a>
 *
 */
#ifndef SOLANA_HEADER
#define SOLANA_HEADER

#include <stdint.h>
#include "coin_utils.h"

// Derivation path reference : https://docs.solana.com/wallet-guide/paper-wallet#hierarchical-derivation
// Taking 3 levels of depth similar to ledger
#define SOLANA_PURPOSE_INDEX 0x8000002C
#define SOLANA_COIN_INDEX    0x800001F5
#define SOLANA_ACCOUNT_INDEX 0x80000000

#define SOLANA_ACCOUNT_ADDRESS_LENGTH 32
#define SOLANA_BLOCKHASH_LENGTH       32

// Reference: https://docs.rs/solana-program/1.14.3/solana_program/system_instruction/enum.SystemInstruction.html
enum SOLANA_SYSTEM_INSTRUCTION {
  SSI_CREATE_ACCOUNT = 0,
  SSI_ASSIGN,
  SSI_TRANSFER,
  SSI_CREATE_ACCOUNT_WITH_SEED,
  SSI_ADVANCE_NONCE_ACCOUNT,
  SSI_WITHDRAW_NONCE_ACCOUNT,
  SSI_INITIALIZE_NONCE_ACCOUNT,
  SSI_AUTHORIZE_NONCE_ACCOUNT,
  SSI_ALLOCATE,
  SSI_ALLOCATE_WITH_SEED,
  SSI_ASSIGN_WITH_SEED,
  SSI_TRANSFER_WITH_SEED,
  SSI_UPGRADE_NONCE_ACCOUNT,
};

enum SOLANA_ERROR_CODES {
  SEC_OK = 0,
  SEC_ERROR,
  SEC_D_MIN_LENGTH,
  SEC_D_COMPACT_U16_OVERFLOW,
  SEC_D_READ_SIZE_MISMATCH,
  SEC_V_UNSUPPORTED_PROGRAM,
  SEC_V_UNSUPPORTED_INSTRUCTION,
  SEC_V_UNSUPPORTED_INSTRUCTION_COUNT,
  SEC_V_INDEX_OUT_OF_RANGE,
  SEC_BU_INVALID_BLOCKHASH,
};

// Reference : https://docs.rs/solana-program/1.14.3/solana_program/system_instruction/enum.SystemInstruction.html#variant.Transfer
typedef struct solana_transfer_data {
  uint8_t *funding_account;
  uint8_t *recipient_account;
  uint64_t lamports;
} solana_transfer_data;

// Reference : https://docs.solana.com/developing/programming-model/transactions#instruction-format
typedef struct solana_instruction {
  uint8_t program_id_index;
  uint16_t account_addresses_index_count;
  uint8_t *account_addresses_index;
  uint16_t opaque_data_length;
  uint8_t *opaque_data;
  union {
    solana_transfer_data transfer;
  } program;
} solana_instruction;

// Reference : https://docs.solana.com/developing/programming-model/transactions#anatomy-of-a-transaction
typedef struct solana_unsigned_txn {
  uint8_t required_signatures_count;
  uint8_t read_only_accounts_require_signature_count;
  uint8_t read_only_accounts_not_require_signature_count;

  uint16_t account_addresses_count;
  uint8_t *account_addresses;

  uint8_t *blockhash;

  uint16_t instructions_count;  // deserialization only supports single instruction
  solana_instruction instruction;

} solana_unsigned_txn;

/**
 * @brief Get the compact array size and number of bytes used to store the size 
 * 
 * @param data the compact array
 * @param size the size of the compact array 
 * @return uint16_t number of bytes used to store the size
 */
uint16_t get_compact_array_size(const uint8_t *data, uint16_t *size, int *error);

/**
 * @brief Convert byte array representation of unsigned transaction to solana_unsigned_txn.
 * @details
 *
 * @param [in] byte_array                   Byte array of unsigned transaction.
 * @param [in] byte_array_size              Size of byte array.
 * @param [out] utxn                        Pointer to the solana_unsigned_txn instance to store the transaction details.
 *
 * @return Status of conversion
 * @retval 0 if successful
 * @retval -1 if unsuccessful
 *
 * @see
 * @since v1.0.0
 *
 * @note
 */
int solana_byte_array_to_unsigned_txn(uint8_t *byte_array, uint16_t byte_array_size, solana_unsigned_txn *utxn);

/**
 * @brief Validate the deserialized unsigned transaction 
 * 
 * @param utxn Pointer to the solana_unsigned_txn instance to validate the transaction
 * @return 0 if validation succeeded
 * @return -1 if validation failed
 */
int solana_validate_unsigned_txn(const solana_unsigned_txn *utxn);

/**
 * @brief Signed unsigned byte array.
 * @details
 *
 * @param [in] unsigned_txn_byte_array      Byte array of unsigned transaction.
 * @param [in] unsigned_txn_len             length of unsigned transaction byte array.
 * @param [in] transaction_metadata         Pointer to txn_metadata instance.
 * @param [in] mnemonics                    char array of mnemonics.
 * @param [in] passphrase                   char array of passphrase.
 * @param [out] sig                         Byte array of signature to store the result of signing unsigned transaction byte array.
 *
 * @return
 * @retval
 *
 * @see
 * @since v1.0.0
 *
 * @note
 */
void solana_sig_unsigned_byte_array(const uint8_t *unsigned_txn_byte_array,
                                    uint64_t unsigned_txn_len,
                                    const txn_metadata *transaction_metadata,
                                    const char *mnemonics,
                                    const char *passphrase,
                                    uint8_t *sig);
/**
 * @brief Update blockhash in serialized array to given blockhash
 * 
 * @param byte_array    pointer to serialized array of unsigned transaction to be modified
 * @param blockhash     pointer to array of latest blockhash
 * @return int 
 */
int solana_update_blockhash_in_byte_array(uint8_t *byte_array, const uint8_t *blockhash);
#endif  // SOLANA_HEADER
