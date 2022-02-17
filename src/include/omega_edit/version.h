/**********************************************************************************************************************
 * Copyright (c) 2021-2022 Concurrent Technologies Corporation.                                                       *
 *                                                                                                                    *
 * Licensed under the Apache License, Version 2.0 (the "License");                                                    *
 * you may not use this file except in compliance with the License.                                                   *
 * You may obtain a copy of the License at                                                                            *
 *                                                                                                                    *
 *     http://www.apache.org/licenses/LICENSE-2.0                                                                     *
 *                                                                                                                    *
 * Unless required by applicable law or agreed to in writing, software                                                *
 * distributed under the License is distributed on an "AS IS" BASIS,                                                  *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.                                           *
 * See the License for the specific language governing permissions and                                                *
 * limitations under the License.                                                                                     *
 **********************************************************************************************************************/

#ifndef OMEGA_EDIT_VERSION_H
#define OMEGA_EDIT_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the major version of the library
 * @return the major version of the library
 */
int omega_version_major();

/**
 * Get the minor version of the library
 * @return the minor version of the library
 */
int omega_version_minor();

/**
 * Get the patch-level of the library
 * @return the patch-level of the library
 */
int omega_version_patch();

/**
 * Get the integer representation of the version of the library
 * @return the integer representation of the version of the library
 */
int omega_version();

#ifdef __cplusplus
}
#endif

#endif//OMEGA_EDIT_VERSION_H