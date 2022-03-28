/**
 * @file    controller_tap_cards.c
 * @author  Cypherock X1 Team
 * @brief   Tap card next controller.
 *          This file contains the functions to control the tap card next controller.
 * @copyright Copyright (c) 2022 HODL TECH PTE LTD
 * <br/> You may obtain a copy of license at <a href="https://mitcc.org/" target=_blank>https://mitcc.org/</a>
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
#include "controller_tap_cards.h"
#include "ui_instruction.h"
#include "nfc.h"
#include "tasks.h"
#include "app_error.h"

NFC_connection_data tap_card_data;

bool tap_card_applet_connection()
{
    uint8_t acceptable_cards;
    if (tap_card_data.desktop_control) {
        set_instant_abort(&abort_from_desktop);
        set_abort_now(&_abort_);
    } else {
        set_instant_abort(NULL);
        set_abort_now(NULL);
    }

    while (1) {
        if (nfc_select_card() != STM_SUCCESS) // Stuck here until card is detected
            return false;

        acceptable_cards = tap_card_data.acceptable_cards;
        tap_card_data.status = nfc_select_applet(tap_card_data.family_id, &tap_card_data.acceptable_cards, NULL, tap_card_data.card_key_id);
        if (tap_card_data.tapped_card != 0 && tap_card_data.tapped_card == (acceptable_cards ^ tap_card_data.acceptable_cards))
            continue;

        /* The tapped_card information should be persistent, as it is used at later stage in the flow
        * For example, in the second half of card-verification, the card tapped in first half becomes the
        * acceptable card for that half.
        * It is the knowledge of previous round of card tapping, hence it's update should happen only here
        * or at the fresh start of a card tap flow. */
        tap_card_data.tapped_card = (acceptable_cards ^ tap_card_data.acceptable_cards);
#if X1WALLET_MAIN
        tap_card_data.keystore_index = is_paired(tap_card_data.card_key_id);
        if (flow_level.level_two != LEVEL_THREE_PAIR_CARD &&
            flow_level.level_two != LEVEL_THREE_VERIFY_CARD &&
            tap_card_data.keystore_index == -1) {
            tap_card_take_to_pairing();
            return false;
        }
#endif

        if (tap_card_data.status == SW_NO_ERROR) {
            const uint8_t *pairing_key = get_keystore_pairing_key(tap_card_data.keystore_index);
        	if (tap_card_data.keystore_index >= 0)
                init_session_keys(pairing_key, pairing_key + 32, NULL);
            return true;
        } else if (tap_card_data.status == SW_CONDITIONS_NOT_SATISFIED) {
            mark_error_screen(ui_text_this_card_is_not_valid);
        } else if (tap_card_data.status == SW_FILE_INVALID) {
            mark_error_screen(ui_text_family_id_mismatch);
        } else if (tap_card_data.status == SW_FILE_NOT_FOUND) {
            mark_error_screen(ui_text_corrupted_card_contact_support);
            reset_flow_level();
        } else if (tap_card_data.status == SW_INCOMPATIBLE_APPLET) {
            mark_error_screen(ui_text_incompatible_card_version);
            reset_flow_level();
        } else {
        	tap_card_data.tapped_card = 0;
        	tap_card_data.acceptable_cards = acceptable_cards;
            if (!(--tap_card_data.retries)) {
                mark_error_screen(ui_text_unknown_error_contact_support);
                reset_flow_level();
            }
        }
        LOG_ERROR("err (0x%02X%02X)\n", tap_card_data.status >> 8, tap_card_data.status & 0xff);

        if (flow_level.show_error_screen) {
            buzzer_start(BUZZER_DURATION);
            if (counter.level != LEVEL_ONE) {   // flow not reset, we wanna retry after prompting user
                flow_level.level_three = tap_card_data.lvl3_retry_point;
                flow_level.level_four = tap_card_data.lvl4_retry_point;
            } else {                            // flow is reset, convey to desktop if needed
                if (tap_card_data.desktop_control)
                    transmit_one_byte_reject(tap_card_data.active_cmd_type);
            }
            instruction_scr_destructor();
            return false;
        }
    }
}

bool tap_card_handle_applet_errors()
{
    LOG_ERROR("err (0x%02X%02X)\n", tap_card_data.status >> 8, tap_card_data.status & 0xff);
    switch (tap_card_data.status) {
        case SW_NO_ERROR: return true;
        case SW_SECURITY_CONDITIONS_NOT_SATISFIED:
            mark_error_screen(ui_text_security_conditions_not_met);
            reset_flow_level();
            break;
        case SW_NOT_PAIRED:
            tap_card_take_to_pairing();
            return true;
        case SW_CONDITIONS_NOT_SATISFIED:
            break;
        case SW_WRONG_DATA:
            mark_error_screen(ui_text_card_invalid_apdu_length);
            reset_flow_level();
            break;
        case SW_FILE_FULL:
            mark_error_screen(ui_text_card_is_full);
            reset_flow_level();
            break;
        case SW_RECORD_NOT_FOUND:
            tap_card_data.active_cmd_type = WALLET_DOES_NOT_EXISTS_ON_CARD;
            mark_error_screen(ui_text_wallet_doesnt_exists_on_this_card);
            reset_flow_level();
            break;
        case SW_TRANSACTION_EXCEPTION:
            mark_error_screen(ui_text_card_transaction_exception);
            reset_flow_level();
            break;
        case SW_NULL_POINTER_EXCEPTION:
            mark_error_screen(ui_text_card_null_pointer_exception);
            reset_flow_level();
            break;
        case SW_OUT_OF_BOUNDARY:
            mark_error_screen(ui_text_card_out_of_boundary_exception);
            reset_flow_level();
            break;
        case SW_INVALID_INS:
            mark_error_screen(ui_text_card_error_contact_support);
            reset_flow_level();
            break;
        default:
            if ((tap_card_data.status & 0xFF00) == POW_SW_WALLET_LOCKED) {
                uint8_t target[SHA256_SIZE], random_number[POW_RAND_NUMBER_SIZE];

                tap_card_data.status = nfc_get_challenge(wallet.wallet_name, target, random_number);

                mark_error_screen(ui_text_wrong_wallet_is_now_locked);
                if (tap_card_data.status == SW_NO_ERROR)
                    add_challenge_flash((const char *) wallet.wallet_name, target, random_number, tap_card_data.tapped_card);
                tap_card_data.active_cmd_type = USER_ENTERED_PIN;
                tap_card_data.lvl3_retry_point = WALLET_LOCKED_MESSAGE;
                flow_level.level_two = LEVEL_THREE_WALLET_LOCKED;
                decrease_level_counter();
            } else if ((tap_card_data.status & 0xFF00) == SW_CORRECT_LENGTH_00) {
                char error_text[40];
                snprintf(error_text, sizeof(error_text), ui_text_wrong_remaining_attempts, tap_card_data.status & 0xFF);
                mark_error_screen(error_text);
                tap_card_data.lvl3_retry_point = flow_level.level_three - 1;
                tap_card_data.lvl4_retry_point = 1;
            } else if ((tap_card_data.status & 0xFF00) == SW_CRYPTO_EXCEPTION) {
                mark_error_screen(ui_text_card_crypto_exception);
                reset_flow_level();
            } else if ((tap_card_data.status & PN532_ERROR_BASE) == PN532_ERROR_BASE) {
                mark_error_screen("NFC hardware issue detected");
                reset_flow_level();
            } else {
            	tap_card_data.tapped_card = 0;
                if (!(--tap_card_data.retries)) {
                    mark_error_screen(ui_text_unknown_error_contact_support);
                    reset_flow_level();
                }
            }
            break;
    }

    if (flow_level.show_error_screen) {
        buzzer_start(BUZZER_DURATION);
        if (counter.level != LEVEL_ONE) {   // flow not reset, we want to retry after prompting user
            flow_level.level_three = tap_card_data.lvl3_retry_point;
            flow_level.level_four = tap_card_data.lvl4_retry_point;
        } else {                            // flow is reset, convey to desktop if needed
            if (tap_card_data.desktop_control)
                transmit_one_byte_reject(tap_card_data.active_cmd_type);
        }
        instruction_scr_destructor();
        return true;
    }

    // probably comm failure; guess is NFC teardown; reconnect with applet
    // silently retry to connect; can't connect here, cards_state is important
    return false;
}

void tap_card_take_to_pairing()
{
#if X1WALLET_MAIN
    buzzer_start(BUZZER_DURATION);
    mark_error_screen(ui_text_device_and_card_not_paired);
    reset_flow_level();
    counter.level = LEVEL_THREE;
    flow_level.level_one = LEVEL_TWO_ADVANCED_SETTINGS;
    flow_level.level_two = LEVEL_THREE_PAIR_CARD;
    if (tap_card_data.desktop_control)
        transmit_one_byte_reject(tap_card_data.active_cmd_type);
    instruction_scr_destructor();
#endif
}