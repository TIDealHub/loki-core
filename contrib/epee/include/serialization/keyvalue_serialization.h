// Copyright (c) 2006-2013, Andrey N. Sabelnikov, www.sabelnikov.net
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
// * Neither the name of the Andrey N. Sabelnikov nor the
// names of its contributors may be used to endorse or promote products
// derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER  BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#pragma once

#include "misc_log_ex.h"
#include "enableable.h"
#include "keyvalue_serialization_overloads.h"
#include "../storages/portable_storage.h"

#undef LOKI_DEFAULT_LOG_CATEGORY
#define LOKI_DEFAULT_LOG_CATEGORY "serialization"

namespace epee
{
  /************************************************************************/
  /* Serialize map declarations                                           */
  /************************************************************************/

/// New way: allows putting serialization implementation into cpp, like this:
///
/// blah.h:
/// class Foo {
///   int val;
///   KV_MAP_SERIALIZABLE
/// }:
///
/// blah.cpp:
/// KV_SERIALIZE_MAP_CODE_BEGIN(Foo)
///   KV_SERIALIZE(val)
/// KV_SERIALIZE_MAP_CODE_END()
#define KV_MAP_SERIALIZABLE \
public: \
  bool store(epee::serialization::portable_storage& st, epee::serialization::section* hparent_section = nullptr) const; \
  bool _load(epee::serialization::portable_storage& st, epee::serialization::section* hparent_section = nullptr); \
  bool load(epee::serialization::portable_storage& st, epee::serialization::section* hparent_section = nullptr); \
  template <bool is_store> bool _serialize_map(epee::serialization::portable_storage& stg, epee::serialization::section* hparent_section) const;

#define KV_SERIALIZE_MAP_CODE_BEGIN(Class) \
  bool Class::store(epee::serialization::portable_storage& st, epee::serialization::section* hparent_section) const \
  { return _serialize_map<true>(st, hparent_section); } \
  bool Class::_load(epee::serialization::portable_storage& st, epee::serialization::section* hparent_section) \
  { return _serialize_map<false>(st, hparent_section); } \
  bool Class::load(epee::serialization::portable_storage& st, epee::serialization::section* hparent_section) \
  { \
    try { return _load(st, hparent_section); } \
    catch (const std::exception& err) { LOG_ERROR("Deserialization exception: " << err.what()); } \
    catch (...) { LOG_ERROR("Unknown deserialization exception"); } \
    return false; \
  } \
  template <bool is_store> \
  bool Class::_serialize_map(epee::serialization::portable_storage& stg, epee::serialization::section* hparent_section) const { \
    /* de-const if we're being called (from the above non-const _load method) to deserialize */ \
    auto& this_ref = const_cast<std::conditional_t<is_store, const Class, Class>&>(*this);

#define KV_SERIALIZE_MAP_CODE_END() return true; }


/// Old deprecated way: puts every last bit of serialization code in the header.  Use this if you
/// are worried about having too much unused memory on your system.
#define BEGIN_KV_SERIALIZE_MAP() \
public: \
  bool store( epee::serialization::portable_storage& st, epee::serialization::section* hparent_section = nullptr) const\
  {\
    return serialize_map<true>(*this, st, hparent_section);\
  }\
  bool _load( epee::serialization::portable_storage& stg, epee::serialization::section* hparent_section = nullptr)\
  {\
    return serialize_map<false>(*this, stg, hparent_section);\
  }\
  bool load( epee::serialization::portable_storage& stg, epee::serialization::section* hparent_section = nullptr)\
  {\
    try{\
    return serialize_map<false>(*this, stg, hparent_section);\
    }\
    catch(const std::exception& err) \
    { \
      (void)(err); \
      LOG_ERROR("Exception on deserializing: " << err.what());\
      return false; \
    }\
  }\
  template<bool is_store, class this_type, class t_storage> \
  static bool serialize_map(this_type& this_ref,  t_storage& stg, typename t_storage::hsection hparent_section) \
  { 

#define KV_SERIALIZE_VALUE(variable) \
  epee::serialization::selector<is_store>::serialize(variable, stg, hparent_section, #variable);

#define KV_SERIALIZE_N(variable, val_name) \
  epee::serialization::selector<is_store>::serialize(this_ref.variable, stg, hparent_section, val_name);

  template<typename T> inline void serialize_default(const T &t, T v) { }
  template<typename T, typename S> inline void serialize_default(T &t, S &&v) { t = std::forward<S>(v); }

#define KV_SERIALIZE_OPT_N(variable, val_name, default_value) \
  do { \
    if (is_store && this_ref.variable == default_value) \
      break; \
    if (!epee::serialization::selector<is_store>::serialize(this_ref.variable, stg, hparent_section, val_name)) \
      epee::serialize_default(this_ref.variable, default_value); \
  } while (0);

#define KV_SERIALIZE_OPT_N2(variable, val_name, default_value) \
do { \
  if (!epee::serialization::selector<is_store>::serialize(this_ref.variable, stg, hparent_section, val_name)) { \
    epee::serialize_default(this_ref.variable, default_value); \
  } else { \
    this_ref.explicitly_set(); \
  } \
} while (0);

#define KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(variable, val_name) \
  epee::serialization::selector<is_store>::serialize_t_val_as_blob(this_ref.variable, stg, hparent_section, val_name); 

#define KV_SERIALIZE_VAL_POD_AS_BLOB_N(variable, val_name) \
  static_assert(std::is_pod<decltype(this_ref.variable)>::value, "t_type must be a POD type."); \
  KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(variable, val_name)

#define KV_SERIALIZE_VAL_POD_AS_BLOB_OPT_N(variable, val_name, default_value) \
  do { \
    static_assert(std::is_pod<decltype(this_ref.variable)>::value, "t_type must be a POD type."); \
    bool ret = KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(variable, val_name); \
    if (!ret) \
      epee::serialize_default(this_ref.variable, default_value); \
  } while(0);

#define KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(variable, val_name) \
  epee::serialization::selector<is_store>::serialize_stl_container_pod_val_as_blob(this_ref.variable, stg, hparent_section, val_name);

#define END_KV_SERIALIZE_MAP() return true;}

#define KV_SERIALIZE(variable)                           KV_SERIALIZE_N(variable, #variable)
#define KV_SERIALIZE_VAL_POD_AS_BLOB(variable)           KV_SERIALIZE_VAL_POD_AS_BLOB_N(variable, #variable)
#define KV_SERIALIZE_VAL_POD_AS_BLOB_OPT(variable, def)  KV_SERIALIZE_VAL_POD_AS_BLOB_OPT_N(variable, #variable, def)
#define KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE(variable)     KV_SERIALIZE_VAL_POD_AS_BLOB_FORCE_N(variable, #variable) //skip is_pod compile time check
#define KV_SERIALIZE_CONTAINER_POD_AS_BLOB(variable)     KV_SERIALIZE_CONTAINER_POD_AS_BLOB_N(variable, #variable)
#define KV_SERIALIZE_OPT(variable,default_value)          KV_SERIALIZE_OPT_N(variable, #variable, default_value)
/// same as KV_SERIALIZE_OPT, but will call `explicitly_set()` if non-defaulted value found
#define KV_SERIALIZE_OPT2(variable,default_value)         KV_SERIALIZE_OPT_N2(variable, #variable, default_value)
#define KV_SERIALIZE_ENUM(enum_) do { \
  using enum_t = std::remove_const_t<decltype(this_ref.enum_)>; \
  using int_t = std::underlying_type_t<enum_t>; \
  int_t int_value = is_store ? static_cast<int_t>(this_ref.enum_) : 0; \
  epee::serialization::selector<is_store>::serialize(int_value, stg, hparent_section, #enum_); \
  if (!is_store) \
    const_cast<enum_t&>(this_ref.enum_) = static_cast<enum_t>(int_value); \
} while(0);

// Stashes `this` in the storage object's context for a dependent type that needs to access it.
#define KV_SERIALIZE_DEPENDENT_N(variable, val_name) do { \
  stg.set_context(&this_ref); \
  KV_SERIALIZE_N(variable, val_name) \
  stg.clear_context(); \
  } while (0);

#define KV_SERIALIZE_DEPENDENT(variable) KV_SERIALIZE_DEPENDENT_N(variable, #variable)

}




