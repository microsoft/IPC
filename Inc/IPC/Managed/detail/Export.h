#pragma once

#ifdef IPC_MANAGED_EXPORT
#define IPC_MANAGED_DLL  __declspec(dllexport)
#else
#define IPC_MANAGED_DLL  __declspec(dllimport)
#endif
