/**
 * @file    core_flow_init.c
 * @author  Cypherock X1 Team
 * @brief
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
#include "core_flow_init.h"

#include <stdint.h>

#include "main_menu.h"

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
 * STATIC VARIABLES
 *****************************************************************************/
flow_step_t *core_step_buffer[10] = {0};
array_list_t core_step_array_list = {0};
const engine_ctx_t core_step_engine_ctx = {.array_list_config =
                                               &core_step_array_list};

/*****************************************************************************
 * GLOBAL VARIABLES
 *****************************************************************************/

/*****************************************************************************
 * STATIC FUNCTION PROTOTYPES
 *****************************************************************************/
/**
 * @brief
 *
 */
void reset_buffers(void);

/*****************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/
void reset_buffers(void) {
  /* Zero-ise the buffer */
  memset(&core_step_buffer[0], 0, sizeof(core_step_buffer));

  /* Set array_list core_step_array_list with appropriate values */
  core_step_array_list.array = &core_step_buffer[0];
  core_step_array_list.current_index = 0;
  core_step_array_list.max_capacity =
      sizeof(core_step_buffer) / sizeof(core_step_buffer[0]);
  core_step_array_list.num_of_elements = 0;
  core_step_array_list.size_of_element = sizeof(core_step_buffer[0]);
}

/*****************************************************************************
 * GLOBAL FUNCTIONS
 *****************************************************************************/
const engine_ctx_t *get_core_flow_ctx(void) {
  reset_buffers();

  // Check onboarding status

  // Check device authentication status

  // Add correct first step of the flow based on the device auth/onboarding
  // status
  engine_add_next_flow_step(&core_step_engine_ctx, main_menu_get_step());

  return &core_step_engine_ctx;
}
