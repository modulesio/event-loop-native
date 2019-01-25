#include <v8.h>
#include <uv.h>
#include <node_api.h>

#ifndef _WIN32
#include <dlfcn.h>
#else
#include <Windows.h>
#endif

// using namespace v8;
// using namespace node;

namespace eventLoopNative {

napi_value getEventLoopAddress(napi_env env, napi_callback_info args) {
  uv_loop_t *loop;
  napi_get_uv_event_loop(env, &loop);

  napi_value result;
  napi_create_array_with_length(env, 2, &result);
  {
    napi_value n;
    napi_create_int32(env, 0xFFFFFFFF & ((uintptr_t)loop >> 32), &n);
    napi_set_element(env, result, 0, n);
  }
  {
    napi_value n;
    napi_create_int32(env, 0xFFFFFFFF & (uintptr_t)loop, &n);
    napi_set_element(env, result, 1, n);
  }

  return result;
}

napi_value doDlclose(napi_env env, napi_callback_info args) {
  size_t argc = 0;
  napi_value argv[1];
  napi_value thisArg;
  void *data;
  napi_get_cb_info(env, args, &argc, argv, &thisArg, &data);

  char soPath[32768];
  size_t length;
  napi_get_value_string_utf8(env, argv[0], soPath, sizeof(soPath), &length);

#ifndef _WIN32
  void *handle = dlopen(soPath, RTLD_LAZY);

  if (handle) {
    while (dlclose(handle) == 0) {}

    napi_value result;
    napi_status napi_get_boolean(napi_env env, true, &result);
    return result;
  } else {
    napi_value result;
    napi_status napi_get_boolean(napi_env env, false, &result);
    return result;
  }
#else
  WCHAR soPath_w[32768];
  MultiByteToWideChar(CP_UTF8, 0, soPath, -1, soPath_w, sizeof(soPath_w)/sizeof(soPath_w[0]));

  HMODULE handle = LoadLibraryExW(soPath_w, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (handle != NULL) {
    while (FreeLibrary(handle)) {}

    napi_value result;
    napi_get_boolean(env, true, &result);
    return result;
  } else {
    napi_value result;
    napi_get_boolean(env, true, &result);
    return result;
  }
#endif

  napi_value undefined;
  napi_get_undefined( env, &undefined);
  return undefined;
}

napi_value Init(napi_env env, napi_value exports) {
  uv_loop_t *loop;
  napi_status status = napi_get_uv_event_loop(env, &loop);
  // uv_async_init(loop, &async, RunInMainThread);
  
  napi_value getEventLoopAddressFn;
  napi_create_function(env, NULL, 0, getEventLoopAddress, NULL, &getEventLoopAddressFn);
  napi_set_named_property(env, exports, "getEventLoopAddress", getEventLoopAddressFn);
  
  napi_value dlcloseFn;
  napi_create_function(env, NULL, 0, doDlclose, NULL, &dlcloseFn);
  napi_set_named_property(env, exports, "dlclose", dlcloseFn);
  
  // std::cout << "do init " << loop << std::endl;
  
  return exports;
}

}

#ifndef LUMIN
NAPI_MODULE(NODE_GYP_MODULE_NAME, eventLoopNative::Init)
#else
extern "C" {
  void node_register_napi_module_worker_native(Local<Object> exports, Local<Value> module, Local<Context> context) {
    eventLoopNative::Init(exports);
  }
}
#endif
