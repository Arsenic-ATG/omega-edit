/**********************************************************************************************************************
 * Copyright (c) 2021 Concurrent Technologies Corporation.                                                            *
 *                                                                                                                    *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance     *
 * with the License.  You may obtain a copy of the License at                                                         *
 *                                                                                                                    *
 *     http://www.apache.org/licenses/LICENSE-2.0                                                                     *
 *                                                                                                                    *
 * Unless required by applicable law or agreed to in writing, software is distributed under the License is            *
 * distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or                   *
 * implied.  See the License for the specific language governing permissions and limitations under the License.       *
 *                                                                                                                    *
 **********************************************************************************************************************/

#ifndef OMEGA_EDIT_VIEWPORT_DEF_HPP
#define OMEGA_EDIT_VIEWPORT_DEF_HPP

#include "../../include/omega_edit/fwd_defs.h"
#include "data_segment_def.hpp"
#include "internal_fwd_defs.hpp"
#include <shared_mutex>

struct omega_viewport_struct {
    omega_session_t *session_ptr{};            ///< Session that owns this viewport instance
    omega_data_segment_t data_segment{};       ///< Viewport data
    omega_viewport_event_cbk_t event_handler{};///< User callback when the viewport changes
    void *user_data_ptr{};                     ///< Pointer to associated user-provided data
    int32_t event_interest_;                   ///< Events of interest
    bool is_floating_{};                       ///< Is this viewport floating with the data or at a fixed offset
    std::shared_mutex viewport_mutex_{};       ///< Viewport mutex
};

#endif//OMEGA_EDIT_VIEWPORT_DEF_HPP
