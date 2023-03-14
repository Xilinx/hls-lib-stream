// (c) Copyright 2021-2022 Xilinx, Inc.
// All Rights Reserved.
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#ifndef X_HLS_NP_CHANNEL_SIM_H
#define X_HLS_NP_CHANNEL_SIM_H

#include "hls_stream.h"
#include <string.h>
namespace hls {
template <typename T, unsigned N_OUT_PORTS, unsigned N_IN_PORTS>
class load_balancing_np : public stream_delegate<sizeof(T)> {
private:
  std::string name;
  std::deque<std::array<char, sizeof(T)> > _data;
#ifdef HLS_STREAM_THREAD_SAFE
  std::mutex _mutex;
  std::condition_variable _condition_var;
  bool invalid;
#endif
protected:
#ifdef HLS_STREAM_THREAD_SAFE
  load_balancing_np(): invalid(false) {}
  load_balancing_np(const char *n) : name(n), invalid(false) {}
  ~load_balancing_np() {
    std::unique_lock<std::mutex> ul(_mutex);
    invalid = true;
    _condition_var.notify_all();
  }
#else
  load_balancing_np() {}
  load_balancing_np(const char *n) : name(n) {}
#endif

public:
  virtual size_t size() {
#ifdef HLS_STREAM_THREAD_SAFE
    std::lock_guard<std::mutex> lg(_mutex);
#endif
    return _data.size();
  }

  virtual bool read(void *elem) {
#ifdef HLS_STREAM_THREAD_SAFE
    std::unique_lock<std::mutex> ul(_mutex);
    stream_globals::incr_blocked_counter();
    while (_data.empty()) {
      while (invalid) { sleep(1); }
      _condition_var.wait(ul);
    }
    stream_globals::decr_blocked_counter();
#else
    if (_data.empty()) {
      std::cout << "WARNING [HLS SIM]: n-port channel '"
                << name
                << "' is read while empty,"
                << " which may result in RTL simulation hanging."
                << std::endl;
      return false;
    }
#endif
    
    memcpy(elem, _data.front().data(), sizeof(T));
    _data.pop_front();
    return true;
  }

  virtual bool read_nb(void *elem) {
#ifdef HLS_STREAM_THREAD_SAFE
    std::lock_guard<std::mutex> lg(_mutex);
#endif
    bool is_empty = _data.empty();
    if (!is_empty) {
      memcpy(elem, _data.front().data(), sizeof(T));
      _data.pop_front();
    }
    return !is_empty;
  }

  virtual void write(const void *elem) {
#ifdef HLS_STREAM_THREAD_SAFE
    std::unique_lock<std::mutex> ul(_mutex);
#endif
    std::array<char, sizeof(T)> elem_data;
    memcpy(elem_data.data(), elem, sizeof(T));
    _data.push_back(elem_data);
    if (stream_globals::get_max_size() < _data.size())
        stream_globals::get_max_size() = _data.size();
#ifdef HLS_STREAM_THREAD_SAFE
    _condition_var.notify_one();
#endif
  }
};

namespace split {
template <typename T, unsigned N_OUT_PORTS, unsigned DEPTH = 2>
class load_balance : public load_balancing_np<T, N_OUT_PORTS, 1> { 
public:
  stream<T> in;
  stream<T> out[N_OUT_PORTS];
  load_balance() {
    in.set_delegate(this);
    for (int i = 0; i < N_OUT_PORTS; i++)
      out[i].set_delegate(this);
  }

  load_balance(const char *name) : load_balancing_np<T, N_OUT_PORTS, 1>(name) {
    in.set_delegate(this);
    for (int i = 0; i < N_OUT_PORTS; i++)
      out[i].set_delegate(this);
  }
};

template <typename T, unsigned N_OUT_PORTS, unsigned ONEPORT_DEPTH = 2, unsigned NPORT_DEPTH = 0>
class round_robin : public stream_delegate<sizeof(T)> {
private:
#ifdef HLS_STREAM_THREAD_SAFE
  std::mutex _mutex;
#endif
  std::string name;
  int pos;
public:
  stream<T> in;
  stream<T> out[N_OUT_PORTS];
  round_robin() : pos(0) {
    in.set_delegate(this);
  }

  round_robin(const char *n) : name(n), pos(0) {
    in.set_delegate(this);
  }

  virtual size_t size() {
    return 0;
  }

  virtual bool read(void *elem) {
    std::cout << "WARNING [HLS SIM]: the 'in' port of n-port channel '"
              << name
              << "' cannot be read."
              << std::endl;
    return false;
  }

  virtual bool read_nb(void *elem) {
    return read(elem);
  }

  virtual void write(const void *elem) {
#ifdef HLS_STREAM_THREAD_SAFE
    std::lock_guard<std::mutex> lg(_mutex);
#endif
    out[pos].write(*(T *)elem);
    pos = (pos + 1) % N_OUT_PORTS;
  }
};
} // end of namespace split

namespace merge {

template <typename T, unsigned N_IN_PORTS, unsigned DEPTH = 2>
class load_balance : public load_balancing_np<T, 1, N_IN_PORTS> { 
public:
  stream<T> in[N_IN_PORTS];
  stream<T> out;
  load_balance() {
    out.set_delegate(this);
    for (int i = 0; i < N_IN_PORTS; i++)
      in[i].set_delegate(this);
  }

  load_balance(const char *name) : load_balancing_np<T, 1, N_IN_PORTS>(name) {
    out.set_delegate(this);
    for (int i = 0; i < N_IN_PORTS; i++)
      in[i].set_delegate(this);
  }
};

template <typename T, unsigned N_IN_PORTS, unsigned ONEPORT_DEPTH = 2, unsigned NPORT_DEPTH = 0>
class round_robin : public stream_delegate<sizeof(T)> { 
private:
#ifdef HLS_STREAM_THREAD_SAFE
  std::mutex _mutex;
#endif
  std::string name;
  int pos;
public:
  stream<T> in[N_IN_PORTS];
  stream<T> out;
  round_robin() : pos(0) {
    out.set_delegate(this);
  }

  round_robin(const char *n) : name(n), pos(0) {
    out.set_delegate(this);
  }

  virtual size_t size() {
    return in[pos].size();
  }

  virtual bool read(void *elem) {
#ifdef HLS_STREAM_THREAD_SAFE
    std::lock_guard<std::mutex> lg(_mutex);
#else
    if (in[pos].empty())
      return false;
#endif
    in[pos].read(*(T *)elem);
    pos = (pos + 1) % N_IN_PORTS;
    return true;
  }

  virtual bool read_nb(void *elem) {
#ifdef HLS_STREAM_THREAD_SAFE
    std::lock_guard<std::mutex> lg(_mutex);
#endif
    if (in[pos].read_nb(*(T *)elem)) {
      pos = (pos + 1) % N_IN_PORTS;
      return true;
    }
    return false; 
  }  

  virtual void write(const void *elem) {
    std::cout << "WARNING [HLS SIM]: the 'out' port of n-port channel '"
              << name
              << "' cannot be written."
              << std::endl;
  }
};
} // end of namespace merge
} // end of namespace hls
#endif
