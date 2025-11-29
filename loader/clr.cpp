#include "clr.h"

#include <windows.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

#ifdef DEBUG_BUILD
#include <cstdio>
#define DBG(fmt, ...) printf("[CLR] " fmt, ##__VA_ARGS__)
#define DBG_HR(msg, hr) printf("[CLR] %s: 0x%08lX\n", msg, (unsigned long)(hr))
#else
#define DBG(fmt, ...)
#define DBG_HR(msg, hr)
#endif

namespace {

const GUID CLSID_MetaHost   = {0x9280188d,0x0e8e,0x4867,{0xb3,0x0c,0x7f,0xa8,0x38,0x84,0xe8,0xde}};
const GUID IID_MetaHost     = {0xD332DB9E,0xB9B3,0x4125,{0x82,0x07,0xA1,0x48,0x84,0xF5,0x32,0x16}};
const GUID IID_RuntimeInfo  = {0xBD39D1D2,0xBA2F,0x486a,{0x89,0xB0,0xB4,0xB0,0xCB,0x46,0x68,0x91}};
const GUID CLSID_Runtime    = {0xCB2F6723,0xAB3A,0x11d2,{0x9C,0x40,0x00,0xC0,0x4F,0xA3,0x0A,0x3E}};
const GUID IID_Runtime      = {0xCB2F6722,0xAB3A,0x11d2,{0x9C,0x40,0x00,0xC0,0x4F,0xA3,0x0A,0x3E}};
const GUID IID_Domain       = {0x05F696DC,0x2B29,0x3663,{0xAD,0x8B,0xC4,0x38,0x9C,0xF2,0xA7,0x13}};
const GUID IID_Assembly     = {0x17156360,0x2F1A,0x384A,{0xBC,0x52,0xFD,0xE9,0x3C,0x21,0x5C,0x5B}};
const GUID IID_MethodInfo   = {0xFFCC1B5D,0xECB8,0x38DD,{0x9B,0x01,0x3D,0xC8,0xAB,0xC2,0xAA,0x5F}};

struct IMetaHost : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetRuntime(LPCWSTR, REFIID, LPVOID*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetVersionFromFile(LPCWSTR, LPWSTR, DWORD*) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumerateInstalledRuntimes(void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumerateLoadedRuntimes(HANDLE, void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE RequestRuntimeLoadedNotification(void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryLegacyV2RuntimeBinding(REFIID, LPVOID*) = 0;
    virtual HRESULT STDMETHODCALLTYPE ExitProcess(INT32) = 0;
};

struct IRuntimeInfo : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE GetVersionString(LPWSTR, DWORD*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRuntimeDirectory(LPWSTR, DWORD*) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsLoaded(HANDLE, BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE LoadErrorString(UINT, LPWSTR, DWORD*, LONG) = 0;
    virtual HRESULT STDMETHODCALLTYPE LoadLibrary(LPCWSTR, HMODULE*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetProcAddress(LPCSTR, LPVOID*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetInterface(REFCLSID, REFIID, LPVOID*) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsLoadable(BOOL*) = 0;
};

struct IRuntime : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CreateLogicalThreadState() = 0;
    virtual HRESULT STDMETHODCALLTYPE DeleteLogicalThreadState() = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchInLogicalThreadState(DWORD*) = 0;
    virtual HRESULT STDMETHODCALLTYPE SwitchOutLogicalThreadState(DWORD**) = 0;
    virtual HRESULT STDMETHODCALLTYPE LocksHeldByLogicalThread(DWORD*) = 0;
    virtual HRESULT STDMETHODCALLTYPE MapFile(HANDLE, HMODULE*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetConfiguration(void**) = 0;
    virtual HRESULT STDMETHODCALLTYPE Start() = 0;
    virtual HRESULT STDMETHODCALLTYPE Stop() = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateDomain(LPCWSTR, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetDefaultDomain(IUnknown**) = 0;
};

struct IDomain : IDispatch {
    virtual HRESULT STDMETHODCALLTYPE get_ToString(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Equals(VARIANT, VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetHashCode(long*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetType(IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE InitializeLifetimeService(VARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetLifetimeService(VARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Evidence(IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_DomainUnload(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_DomainUnload(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_AssemblyLoad(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_AssemblyLoad(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ProcessExit(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ProcessExit(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_TypeResolve(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_TypeResolve(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_ResourceResolve(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_ResourceResolve(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_AssemblyResolve(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_AssemblyResolve(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE add_UnhandledException(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE remove_UnhandledException(IUnknown*) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly(IUnknown*, int, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly_2(IUnknown*, int, BSTR, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly_3(IUnknown*, int, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly_4(IUnknown*, int, IUnknown*, IUnknown*, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly_5(IUnknown*, int, BSTR, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly_6(IUnknown*, int, BSTR, IUnknown*, IUnknown*, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly_7(IUnknown*, int, IUnknown*, IUnknown*, IUnknown*, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly_8(IUnknown*, int, BSTR, IUnknown*, IUnknown*, IUnknown*, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE DefineDynamicAssembly_9(IUnknown*, int, BSTR, IUnknown*, IUnknown*, IUnknown*, IUnknown*, VARIANT_BOOL, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateInstance(BSTR, BSTR, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateInstanceFrom(BSTR, BSTR, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateInstance_2(BSTR, BSTR, SAFEARRAY*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateInstanceFrom_2(BSTR, BSTR, SAFEARRAY*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateInstance_3(BSTR, BSTR, VARIANT_BOOL, int, IUnknown*, SAFEARRAY*, IUnknown*, SAFEARRAY*, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateInstanceFrom_3(BSTR, BSTR, VARIANT_BOOL, int, IUnknown*, SAFEARRAY*, IUnknown*, SAFEARRAY*, IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE Load(IUnknown*, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE Load_2(BSTR, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE Load_3(SAFEARRAY*, IUnknown**) = 0;
};

struct IAssembly : IDispatch {
    virtual HRESULT STDMETHODCALLTYPE get_ToString(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Equals(VARIANT, VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetHashCode(long*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetType(IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_CodeBase(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_EscapedCodeBase(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetName(IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetName_2(VARIANT_BOOL, IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_FullName(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_EntryPoint(IUnknown**) = 0;
};

struct IMethodInfo : IDispatch {
    virtual HRESULT STDMETHODCALLTYPE get_ToString(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Equals(VARIANT, VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetHashCode(long*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetType(IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_MemberType(int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_name(BSTR*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_DeclaringType(IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ReflectedType(IUnknown**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCustomAttributes(IUnknown*, VARIANT_BOOL, SAFEARRAY**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetCustomAttributes_2(VARIANT_BOOL, SAFEARRAY**) = 0;
    virtual HRESULT STDMETHODCALLTYPE IsDefined(IUnknown*, VARIANT_BOOL, VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetParameters(SAFEARRAY**) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMethodImplementationFlags(int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_MethodHandle(void*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_Attributes(int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_CallingConvention(int*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Invoke_2(VARIANT, int, IUnknown*, SAFEARRAY*, IUnknown*, VARIANT*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsPublic(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsPrivate(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsFamily(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsAssembly(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsFamilyAndAssembly(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsFamilyOrAssembly(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsStatic(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsFinal(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsVirtual(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsHideBySig(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsAbstract(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsSpecialName(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_IsConstructor(VARIANT_BOOL*) = 0;
    virtual HRESULT STDMETHODCALLTYPE Invoke_3(VARIANT, SAFEARRAY*, VARIANT*) = 0;
};

typedef HRESULT(WINAPI* CLRCreateInstanceFn)(REFCLSID, REFIID, LPVOID*);

IRuntime* g_Runtime = nullptr;
IDomain*  g_Domain  = nullptr;
HMODULE   g_Mscoree = nullptr;

} // anonymous namespace

namespace clr {

bool Initialize() {
    DBG("CoInitializeEx...\n");
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    
    DBG("Loading mscoree.dll...\n");
    g_Mscoree = LoadLibraryA("mscoree.dll");
    if (!g_Mscoree) {
        DBG("  Failed to load mscoree.dll: %lu\n", GetLastError());
        return false;
    }
    
    DBG("Getting CLRCreateInstance...\n");
    auto fn = (CLRCreateInstanceFn)GetProcAddress(g_Mscoree, "CLRCreateInstance");
    if (!fn) {
        DBG("  CLRCreateInstance not found\n");
        return false;
    }
    
    DBG("Creating MetaHost...\n");
    IMetaHost* meta = nullptr;
    HRESULT hr = fn(CLSID_MetaHost, IID_MetaHost, (void**)&meta);
    if (FAILED(hr)) {
        DBG_HR("CLRCreateInstance failed", hr);
        return false;
    }
    
    DBG("Getting runtime v4.0.30319...\n");
    IRuntimeInfo* info = nullptr;
    hr = meta->GetRuntime(L"v4.0.30319", IID_RuntimeInfo, (void**)&info);
    meta->Release();
    if (FAILED(hr)) {
        DBG_HR("GetRuntime failed", hr);
        return false;
    }
    
    DBG("Getting ICorRuntimeHost interface...\n");
    hr = info->GetInterface(CLSID_Runtime, IID_Runtime, (void**)&g_Runtime);
    info->Release();
    if (FAILED(hr)) {
        DBG_HR("GetInterface failed", hr);
        return false;
    }
    
    DBG("Starting runtime...\n");
    hr = g_Runtime->Start();
    if (FAILED(hr)) {
        DBG_HR("Start failed", hr);
        return false;
    }
    
    DBG("Getting default AppDomain...\n");
    IUnknown* domainUnk = nullptr;
    hr = g_Runtime->GetDefaultDomain(&domainUnk);
    if (FAILED(hr)) {
        DBG_HR("GetDefaultDomain failed", hr);
        return false;
    }
    
    DBG("QueryInterface for _AppDomain...\n");
    hr = domainUnk->QueryInterface(IID_Domain, (void**)&g_Domain);
    domainUnk->Release();
    if (FAILED(hr)) {
        DBG_HR("QueryInterface failed", hr);
        return false;
    }
    
    DBG("CLR initialized successfully.\n");
    return true;
}

bool Execute(const std::vector<uint8_t>& assembly) {
    if (!g_Domain) {
        DBG("No domain available\n");
        return false;
    }
    
    DBG("Creating SAFEARRAY (%zu bytes)...\n", assembly.size());
    SAFEARRAYBOUND bounds = { (ULONG)assembly.size(), 0 };
    SAFEARRAY* arr = SafeArrayCreate(VT_UI1, 1, &bounds);
    if (!arr) {
        DBG("  SafeArrayCreate failed\n");
        return false;
    }
    
    void* data = nullptr;
    HRESULT hr = SafeArrayAccessData(arr, &data);
    if (FAILED(hr)) {
        DBG_HR("SafeArrayAccessData failed", hr);
        SafeArrayDestroy(arr);
        return false;
    }
    
    memcpy(data, assembly.data(), assembly.size());
    SafeArrayUnaccessData(arr);
    
    DBG("Loading assembly via AppDomain.Load_3...\n");
    IUnknown* asmUnk = nullptr;
    hr = g_Domain->Load_3(arr, &asmUnk);
    SafeArrayDestroy(arr);
    if (FAILED(hr)) {
        DBG_HR("Load_3 failed", hr);
        return false;
    }
    
    DBG("QueryInterface for _Assembly...\n");
    IAssembly* asm_ = nullptr;
    hr = asmUnk->QueryInterface(IID_Assembly, (void**)&asm_);
    asmUnk->Release();
    if (FAILED(hr)) {
        DBG_HR("QueryInterface _Assembly failed", hr);
        return false;
    }
    
#ifdef DEBUG_BUILD
    BSTR fullName = nullptr;
    if (SUCCEEDED(asm_->get_FullName(&fullName)) && fullName) {
        wprintf(L"[CLR] Assembly: %s\n", fullName);
        SysFreeString(fullName);
    }
#endif
    
    DBG("Getting entry point...\n");
    IUnknown* entryUnk = nullptr;
    hr = asm_->get_EntryPoint(&entryUnk);
    if (FAILED(hr) || !entryUnk) {
        DBG_HR("get_EntryPoint failed", hr);
        asm_->Release();
        return false;
    }
    
    DBG("QueryInterface for _MethodInfo...\n");
    IMethodInfo* method = nullptr;
    hr = entryUnk->QueryInterface(IID_MethodInfo, (void**)&method);
    entryUnk->Release();
    if (FAILED(hr)) {
        DBG_HR("QueryInterface _MethodInfo failed", hr);
        asm_->Release();
        return false;
    }
    
#ifdef DEBUG_BUILD
    BSTR methodName = nullptr;
    if (SUCCEEDED(method->get_name(&methodName)) && methodName) {
        wprintf(L"[CLR] Entry point: %s\n", methodName);
        SysFreeString(methodName);
    }
#endif
    
    DBG("Checking parameters...\n");
    SAFEARRAY* paramsInfo = nullptr;
    method->GetParameters(&paramsInfo);
    int paramCount = 0;
    if (paramsInfo) {
        LONG lb, ub;
        SafeArrayGetLBound(paramsInfo, 1, &lb);
        SafeArrayGetUBound(paramsInfo, 1, &ub);
        paramCount = ub - lb + 1;
        SafeArrayDestroy(paramsInfo);
    }
    DBG("  Parameter count: %d\n", paramCount);
    
    DBG("Building invoke arguments...\n");
    SAFEARRAY* args = nullptr;
    if (paramCount == 0) {
        SAFEARRAYBOUND b = { 0, 0 };
        args = SafeArrayCreate(VT_VARIANT, 1, &b);
    } else {
        SAFEARRAYBOUND sb = { 0, 0 };
        SAFEARRAY* strArr = SafeArrayCreate(VT_BSTR, 1, &sb);
        SAFEARRAYBOUND pb = { 1, 0 };
        args = SafeArrayCreate(VT_VARIANT, 1, &pb);
        VARIANT v;
        VariantInit(&v);
        v.vt = VT_ARRAY | VT_BSTR;
        v.parray = strArr;
        LONG idx = 0;
        SafeArrayPutElement(args, &idx, &v);
    }
    
    DBG("Invoking entry point...\n");
    VARIANT obj, result;
    VariantInit(&obj);
    VariantInit(&result);
    obj.vt = VT_NULL;
    
    hr = method->Invoke_3(obj, args, &result);
    DBG_HR("Invoke_3 result", hr);
    
    VariantClear(&result);
    if (args) SafeArrayDestroy(args);
    method->Release();
    asm_->Release();
    
    return SUCCEEDED(hr);
}

void Shutdown() {
    DBG("Shutting down CLR...\n");
    if (g_Domain)  { g_Domain->Release();  g_Domain  = nullptr; }
    if (g_Runtime) { g_Runtime->Stop(); g_Runtime->Release(); g_Runtime = nullptr; }
    if (g_Mscoree) { FreeLibrary(g_Mscoree); g_Mscoree = nullptr; }
    CoUninitialize();
    DBG("Shutdown complete.\n");
}

} // namespace clr
