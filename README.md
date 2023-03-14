# HLS Stream Library

This directory provides the C simulation model of HLS stream library.

Header file          | Description      
---------------------|------------------
hls_stream.h | Class hls::stream to model the stream connecting two concurrent processes
hls_np_channel.h | hls::split and hls::merge to model the stream connecting multiple concurrent processes by one-to-N or N-to-one style
hls_streamofblocks | Class hls::stream_of_blocks to model the stream of array-like objects

## Compatibility

Tested with g++ (GCC) 8.3.0 on x86_64 GNU/Linux.

## Usage

Include the `hls_stream.h`, `hls_np_channel.h` or `hls_streamofblocks.h` in C++ code.

## License Info

(c) Copyright 2011-2022 Xilinx, Inc.
All Rights Reserved.

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
