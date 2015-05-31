// wrapper for dlldata.c

#ifdef _MERGE_PROXYSTUB // merge proxy stub DLL

#define REGISTER_PROXY_DLL //DllRegisterServer, etc.

#define _WIN32_WINNT 0x0500	//for WinNT 4.0 or Win95 with DCOM
//#define USE_STUBLESS_PROXY	//defined only with MIDL switch /Oicf

#pragma comment(lib, "rpcns4.lib")
#pragma comment(lib, "rpcrt4.lib")

#define ENTRY_PREFIX	Prx

#ifdef _WIN64
  #include "dlldata64.c"
  #include "RemoteBridge_p64.c"
#else //_WIN64
  #include "dlldata.c"
  #include "RemoteBridge_p.c"
#endif //_WIN64

#endif //_MERGE_PROXYSTUB
