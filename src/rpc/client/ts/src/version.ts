/*
 * Copyright (c) 2021 Concurrent Technologies Corporation.
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { client } from './settings'
import { Empty } from 'google-protobuf/google/protobuf/empty_pb'

export function getVersion(): Promise<string> {
  return new Promise<string>((resolve, reject) => {
    client.getVersion(new Empty(), (err, v) => {
      if (err) {
        console.log(err.message)
        return reject('getVersion error: ' + err.message)
      }

      if (!v) {
        console.log('undefined version')
        return reject('undefined version')
      }

      return resolve(`v${v.getMajor()}.${v.getMinor()}.${v.getPatch()}`)
    })
  })
}
