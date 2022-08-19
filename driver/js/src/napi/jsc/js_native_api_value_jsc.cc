/*
 *
 * Tencent is pleased to support the open source community by making
 * Hippy available.
 *
 * Copyright (C) 2019 THL A29 Limited, a Tencent company.
 * All rights reserved.
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
 *
 */

#include <limits.h>

#include <iostream>
#include "driver/napi/js_native_api.h"
#include "driver/napi/jsc/js_native_api_jsc.h"
#include "driver/napi/jsc/js_native_jsc_helper.h"
#include "driver/napi/native_source_code.h"
#include "footstone/logging.h"
#include "footstone/string_view_utils.h"

namespace hippy {
inline namespace driver {
inline namespace napi {

using string_view = footstone::stringview::string_view;
using StringViewUtils = footstone::stringview::StringViewUtils;

bool JSCCtx::GetValueNumber(const std::shared_ptr<CtxValue>& value, double* result) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value = std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  if (JSValueIsNumber(context_, value_ref)) {
    JSValueRef exception = nullptr;
    *result = JSValueToNumber(context_, value_ref, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return false;
    }
    return true;
  }

  return false;
}

bool JSCCtx::GetValueNumber(const std::shared_ptr<CtxValue>& value, int32_t* result) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value = std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  if (JSValueIsNumber(context_, value_ref)) {
    JSValueRef exception = nullptr;
    *result = JSValueToNumber(context_, value_ref, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return false;
    }
    return true;
  }

  return false;
}

bool JSCCtx::GetValueBoolean(const std::shared_ptr<CtxValue>& value, bool* result) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  if (JSValueIsBoolean(context_, value_ref)) {
    *result = JSValueToBoolean(context_, value_ref);
    return true;
  }

  return false;
}

bool JSCCtx::GetValueString(const std::shared_ptr<CtxValue>& value,
                            string_view* result) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  if (JSValueIsString(context_, value_ref)) {
    JSValueRef exception = nullptr;
    JSStringRef str_ref = JSValueToStringCopy(context_, value_ref, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return false;
    }
    *result = string_view(
        reinterpret_cast<const char16_t*>(JSStringGetCharactersPtr(str_ref)),
        JSStringGetLength(str_ref));
    return true;
  }

  return false;
}

bool JSCCtx::IsArray(const std::shared_ptr<CtxValue>& value) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  return JSValueIsArray(context_, value_ref);
}

uint32_t JSCCtx::GetArrayLength(const std::shared_ptr<CtxValue>& value) {
  if (!value) {
    return 0;
  }
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  JSValueRef exception = nullptr;
  JSObjectRef array = JSValueToObject(context_, value_ref, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return 0;
  }
  // to do
  JSStringRef prop_name = JSStringCreateWithCharacters(
    reinterpret_cast<const JSChar*>(kLengthStr), ARRAY_SIZE(kLengthStr) - 1);
  exception = nullptr;
  JSValueRef val = JSObjectGetProperty(context_, array, prop_name, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return 0;
  }
  JSStringRelease(prop_name);
  exception = nullptr;
  uint32_t count = JSValueToNumber(context_, val, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return 0;
  }
  return count;
}

bool JSCCtx::GetValueJson(const std::shared_ptr<CtxValue>& value,
                          string_view* result) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  JSValueRef exception = nullptr;
  JSStringRef str_ref =
      JSValueCreateJSONString(context_, value_ref, 0, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return false;
  }
  *result = string_view(
      reinterpret_cast<const char16_t*>(JSStringGetCharactersPtr(str_ref)),
      JSStringGetLength(str_ref));
  JSStringRelease(str_ref);
  return true;
}

bool JSCCtx::GetEntriesFromObject(const std::shared_ptr<CtxValue>& value,
                                  std::unordered_map<string_view, std::shared_ptr<CtxValue>> &map) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  if (!JSValueIsObject(context_, value_ref)) {
    return false;
  }
  JSValueRef exception = nullptr;
  JSObjectRef obj_value = JSValueToObject(context_, value_ref, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return false;
  }
  JSPropertyNameArrayRef name_arry = JSObjectCopyPropertyNames(context_, obj_value);
  size_t len = JSPropertyNameArrayGetCount(name_arry);
  for (uint32_t i = 0; i < len; ++i) {
    JSStringRef props_key = JSPropertyNameArrayGetNameAtIndex(name_arry, i);
    JSValueRef props_value = JSObjectGetProperty(context_, obj_value, props_key, nullptr);
    const char16_t* utf16_string = reinterpret_cast<const char16_t*>(JSStringGetCharactersPtr(props_key));
    size_t str_length = JSStringGetLength(props_key);
    string_view string_view(utf16_string, str_length);
    auto value = std::make_shared<JSCCtxValue>(context_, props_value);
    map[string_view] = value;
  }
  JSPropertyNameArrayRelease(name_arry);

  return true;
}

bool JSCCtx::HasNamedProperty(const std::shared_ptr<CtxValue>& value,
                              const string_view& name) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  JSValueRef exception = nullptr;
  JSObjectRef object = JSValueToObject(context_, value_ref, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return false;
  }
  JSStringRef property_name = CreateJSCString(name);
  bool ret = JSObjectHasProperty(context_, object, property_name);
  JSStringRelease(property_name);
  return ret;
}

bool JSCCtx::IsFunction(const std::shared_ptr<CtxValue>& value) {
  if (!value) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  if (!JSValueIsObject(context_, value_ref)) {
    return false;
  }

  JSValueRef exception = nullptr;
  JSObjectRef object = JSValueToObject(context_, value_ref, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return false;
  }
  return JSObjectIsFunction(context_, object);
}

string_view JSCCtx::CopyFunctionName(
    const std::shared_ptr<CtxValue>& function) {
  FOOTSTONE_UNIMPLEMENTED();
  return "";
}

std::shared_ptr<CtxValue> JSCCtx::CreateNumber(double number) {
  JSValueRef value = JSValueMakeNumber(context_, number);
  return std::make_shared<JSCCtxValue>(context_, value);
}

std::shared_ptr<CtxValue> JSCCtx::CreateBoolean(bool b) {
  JSValueRef value = JSValueMakeBoolean(context_, b);
  return std::make_shared<JSCCtxValue>(context_, value);
}

std::shared_ptr<CtxValue> JSCCtx::CreateString(
    const string_view &str_view) {
  JSStringRef str_ref = CreateJSCString(str_view);
  JSValueRef value = JSValueMakeString(context_, str_ref);
  JSStringRelease(str_ref);
  return std::make_shared<JSCCtxValue>(context_, value);
}

std::shared_ptr<CtxValue> JSCCtx::CreateUndefined() {
  JSValueRef value = JSValueMakeUndefined(context_);
  return std::make_shared<JSCCtxValue>(context_, value);
}

std::shared_ptr<CtxValue> JSCCtx::CreateNull() {
  JSValueRef value = JSValueMakeNull(context_);
  return std::make_shared<JSCCtxValue>(context_, value);
}

std::shared_ptr<CtxValue> JSCCtx::ParseJson(const string_view& json) {
  JSStringRef str_ref = CreateJSCString(json);
  JSValueRef value = JSValueMakeFromJSONString(context_, str_ref);
  JSStringRelease(str_ref);
  return std::make_shared<JSCCtxValue>(context_, value);
}

std::shared_ptr<CtxValue> JSCCtx::CreateObject(const std::unordered_map<
    string_view,
    std::shared_ptr<CtxValue>>& object) {
  std::unordered_map<std::shared_ptr<CtxValue>,std::shared_ptr<CtxValue>> obj;
  for (const auto& it : object) {
    auto key = CreateString(it.first);
    obj[key] = it.second;
  }
  return CreateObject(obj);
}

std::shared_ptr<CtxValue> JSCCtx::CreateObject(const std::unordered_map<std::shared_ptr<CtxValue>, std::shared_ptr<CtxValue>> &object) {
  JSObjectRef obj = JSObjectMake(context_, nullptr, nullptr);
  JSValueRef exception = nullptr;
  for (const auto& it : object) {
    string_view key;
    auto flag = GetValueString(it.first, &key);
    FOOTSTONE_DCHECK(flag);
    if (!flag) {
      auto error = CreateError("CreateObject");
      SetException(std::static_pointer_cast<JSCCtxValue>(error));
      return nullptr;
    }
    auto object_key = CreateJSCString(key);
    auto ctx_value = std::static_pointer_cast<JSCCtxValue>(it.second);
    JSObjectSetProperty(context_, obj, object_key, ctx_value->value_, kJSPropertyAttributeNone, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return nullptr;
    }
  }
  return std::make_shared<JSCCtxValue>(context_, obj);
}

std::shared_ptr<CtxValue> JSCCtx::CreateArray(
    size_t count,
    std::shared_ptr<CtxValue> array[]) {
  if (count <= 0) {
    return nullptr;
  }

  JSValueRef values[count];  // NOLINT(runtime/arrays)
  for (size_t i = 0; i < count; i++) {
    std::shared_ptr<JSCCtxValue> ele_value =
        std::static_pointer_cast<JSCCtxValue>(array[i]);
    values[i] = ele_value->value_;
  }

  JSValueRef exception = nullptr;
  JSValueRef value_ref = JSObjectMakeArray(context_, count, values, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return nullptr;
  }
  return std::make_shared<JSCCtxValue>(context_, value_ref);
}

void JSCCtx_dataBufferFree(void* bytes, void* deallocatorContext) {
  free(bytes);
}

std::shared_ptr<CtxValue> JSCCtx::CreateByteBuffer(const void* buffer, size_t length) {
  if (nullptr == buffer || 0 == length) {
    return nullptr;
  }
  void* data = malloc(length);
  if (!data) {
    FOOTSTONE_DLOG(ERROR) << "malloc failure, Out of memory";
    return nullptr;
  }
  memcpy(data, buffer, length);
  JSValueRef exception = nullptr;
  JSValueRef value_ref = JSObjectMakeArrayBufferWithBytesNoCopy(context_, data, length, JSCCtx_dataBufferFree, nullptr, &exception);
  if (exception) {
    SetException(std::make_shared<hippy::napi::JSCCtxValue>(context_, exception));
    return nullptr;
  }
  return std::make_shared<hippy::napi::JSCCtxValue>(context_, value_ref);
}

bool JSCCtx::GetByteBuffer(const std::shared_ptr<CtxValue>& value,
                           void** out_data,
                           size_t& out_length,
                           uint32_t& out_type) {
  if (!value || *out_data) {
    return false;
  }
  std::shared_ptr<JSCCtxValue> ctx_value = std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  JSObjectRef object_ref = JSValueToObject(context_, value_ref, nullptr);
  JSTypedArrayType type = JSValueGetTypedArrayType(context_, value_ref, nullptr);
  JSValueRef exception = nullptr;
  if (kJSTypedArrayTypeArrayBuffer == type) {
    *out_data = JSObjectGetArrayBufferBytesPtr(context_, object_ref, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return false;
    }
    out_length = JSObjectGetArrayBufferByteLength(context_, object_ref, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return false;
    }
  }
  else if (kJSTypedArrayTypeNone != type) {
    *out_data = JSObjectGetTypedArrayBytesPtr(context_, object_ref, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return false;
    }
    out_length = JSObjectGetTypedArrayByteLength(context_, object_ref, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return false;
    }
  }
  return true;
}


std::shared_ptr<CtxValue> JSCCtx::CreateError(
    const string_view& msg) {
  JSStringRef str_ref = CreateJSCString(msg);
  JSValueRef value = JSValueMakeString(context_, str_ref);
  JSStringRelease(str_ref);
  JSValueRef values[] = {value};
  JSObjectRef error = JSObjectMakeError(context_, 1, values, nullptr);
  return std::make_shared<JSCCtxValue>(context_, error);
}

std::shared_ptr<CtxValue> JSCCtx::CopyArrayElement(
    const std::shared_ptr<CtxValue>& array,
    uint32_t index) {
  std::shared_ptr<JSCCtxValue> array_value =
      std::static_pointer_cast<JSCCtxValue>(array);
  uint32_t count = GetArrayLength(array_value);
  if (count <= 0 || index >= count) {
    return nullptr;
  }

  JSValueRef exception = nullptr;
  JSValueRef value_ref = array_value->value_;
  JSObjectRef array_ref = JSValueToObject(context_, value_ref, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return nullptr;
  }
  exception = nullptr;
  JSValueRef element =
      JSObjectGetPropertyAtIndex(context_, array_ref, index, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return nullptr;
  }
  return std::make_shared<JSCCtxValue>(context_, element);
}

std::shared_ptr<CtxValue> JSCCtx::CopyNamedProperty(
    const std::shared_ptr<CtxValue>& value,
    const string_view& name) {
  std::shared_ptr<JSCCtxValue> ctx_value =
      std::static_pointer_cast<JSCCtxValue>(value);
  JSValueRef value_ref = ctx_value->value_;
  JSValueRef exception = nullptr;
  JSObjectRef object = JSValueToObject(context_, value_ref, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return nullptr;
  }
  JSStringRef property_name = CreateJSCString(name);
  exception = nullptr;
  JSValueRef property =
      JSObjectGetProperty(context_, object, property_name, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return nullptr;
  }
  JSStringRelease(property_name);
  if (JSValueIsNull(context_, property) ||
      JSValueIsUndefined(context_, property)) {
    return nullptr;
  }

  return std::make_shared<JSCCtxValue>(context_, property);
}

std::shared_ptr<CtxValue> JSCCtx::CallFunction(
    const std::shared_ptr<CtxValue>& function,
    size_t argc,
    const std::shared_ptr<CtxValue> args[]) {
  std::shared_ptr<JSCCtxValue> func_value =
      std::static_pointer_cast<JSCCtxValue>(function);
  JSValueRef func_ref = func_value->value_;
  JSValueRef exception = nullptr;
  JSObjectRef object = JSValueToObject(context_, func_ref, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return nullptr;
  }
  if (argc <= 0) {
    JSValueRef ret_value_ref = JSObjectCallAsFunction(context_, object, nullptr,
                                                      0, nullptr, &exception);
    if (exception) {
      SetException(std::make_shared<JSCCtxValue>(context_, exception));
      return nullptr;
    }
    return std::make_shared<JSCCtxValue>(context_, ret_value_ref);
  }

  JSValueRef values[argc];  // NOLINT(runtime/arrays)
  for (size_t i = 0; i < argc; i++) {
    std::shared_ptr<JSCCtxValue> arg_value =
        std::static_pointer_cast<JSCCtxValue>(args[i]);
    values[i] = arg_value->value_;
  }

  JSValueRef ret_value_ref = JSObjectCallAsFunction(context_, object, nullptr,
                                                    argc, values, &exception);
  if (exception) {
    SetException(std::make_shared<JSCCtxValue>(context_, exception));
    return nullptr;
  }

  if (!ret_value_ref) {
    return nullptr;
  }

  return std::make_shared<JSCCtxValue>(context_, ret_value_ref);
}

string_view JSCCtx::GetExceptionMsg(
    const std::shared_ptr<CtxValue>& exception) {
  if (!exception) {
    return string_view();
  }

  std::shared_ptr<CtxValue> msg_obj = CopyNamedProperty(
    exception, string_view(kMessageStr, ARRAY_SIZE(kMessageStr) - 1));
  string_view msg_view;
  GetValueString(msg_obj, &msg_view);
  std::u16string u16_msg;
  if (!StringViewUtils::IsEmpty(msg_view)) {
    u16_msg = msg_view.utf16_value();
  }
  std::shared_ptr<CtxValue> stack_obj = CopyNamedProperty(
    exception, string_view(kStackStr, ARRAY_SIZE(kStackStr) - 1));
  string_view stack_view;
  GetValueString(stack_obj, &stack_view);
  std::u16string u16_stack;
  if (!StringViewUtils::IsEmpty(stack_view)) {
    u16_stack = stack_view.utf16_value();
  }
  std::u16string str = u"message: " + u16_msg + u", stack: " + u16_stack;
  string_view ret(str.c_str(), str.length());
  FOOTSTONE_DLOG(ERROR) << "GetExceptionMsg msg = " << ret;
  return ret;
}

void JSCCtx::ThrowException(const std::shared_ptr<CtxValue> &exception) {
  SetException(std::static_pointer_cast<JSCCtxValue>(exception));
}

void JSCCtx::ThrowException(const string_view& exception) {
  ThrowException(CreateError(exception));
}

void JSCCtx::HandleUncaughtException(const std::shared_ptr<CtxValue>& exception) {
  if (!exception) {
    return;
  }

  std::shared_ptr<CtxValue> exception_handler =
      GetGlobalObjVar(kHippyErrorHandlerName);
  if (!IsFunction(exception_handler)) {
    const auto& source_code = hippy::GetNativeSourceCode(kErrorHandlerJSName);
    FOOTSTONE_DCHECK(source_code.data_ && source_code.length_);
    string_view content(source_code.data_, source_code.length_);
    exception_handler = RunScript(content, kErrorHandlerJSName);
    bool is_func = IsFunction(exception_handler);
    FOOTSTONE_CHECK(is_func)
        << "HandleUncaughtJsError ExceptionHandle.js don't return function!!!";
    SetGlobalObjVar(kHippyErrorHandlerName, exception_handler,
                    PropertyAttribute::ReadOnly);
  }

  std::shared_ptr<CtxValue> args[2];
  args[0] = CreateString("uncaughtException");
  args[1] = exception;
  CallFunction(exception_handler, 2, args);
}

JSStringRef JSCCtx::CreateJSCString(const string_view& str_view) {
  string_view::Encoding encoding = str_view.encoding();
  JSStringRef ret;
  switch (encoding) {
    case string_view::Encoding::Unknown: {
      FOOTSTONE_UNREACHABLE();
      break;
    }
    case string_view::Encoding::Latin1: {
      ret = JSStringCreateWithUTF8CString(str_view.latin1_value().c_str());
      break;
    }
    case string_view::Encoding::Utf8: {
      std::string u8_str(
          reinterpret_cast<const char*>(str_view.utf8_value().c_str()),
          str_view.utf8_value().length());
      ret = JSStringCreateWithUTF8CString(u8_str.c_str());
      break;
    }
    case string_view::Encoding::Utf16: {
      std::u16string u16_str = str_view.utf16_value();
      ret = JSStringCreateWithCharacters(
          reinterpret_cast<const JSChar*>(u16_str.c_str()), u16_str.length());
      break;
    }
    case string_view::Encoding::Utf32: {
      std::u16string u16_str = StringViewUtils::ConvertEncoding(str_view, string_view::Encoding::Utf16).utf16_value();
      ret = JSStringCreateWithCharacters(
          reinterpret_cast<const JSChar*>(u16_str.c_str()), u16_str.length());
      break;
    }
    default:
      FOOTSTONE_UNIMPLEMENTED();
      break;
  }
  return ret;
}

} // namespace napi
} // namespace driver
} // namespace hippy

