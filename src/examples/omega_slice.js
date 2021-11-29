/*
* Copyright 2021 Concurrent Technologies Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

/*
 * Uses Omega Edit to extract and save a segment from a file.  Example:
 * node ./omega_slice.js ../../LICENSE.txt LICENSE.2-3.txt 2 3
 * cat LICENSE.2-3.txt
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 */
omega_edit = require('../../build/Release/omega_edit')
input_filename = process.argv[2]
output_filename = process.argv[3]
offset = parseInt(process.argv[3])
length = parseInt(process.argv[4])
// TODO: Solve "Illegal arguments for function create_session."
session = omega_edit.create_session(input_filename, null, null, 0, offset, length)
omega_edit.save_session(session, output_filename)
omega_edit.destroy_session(session)
console.log("finished!")