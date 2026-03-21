#pragma once
#include "ChiliWin.h"
#include <sal.h>

#ifdef __cplusplus
extern "C" {
#endif
	const WCHAR* WINAPI DXGetErrorStringW(_In_ HRESULT hr);
	const CHAR* WINAPI DXGetErrorStringA(_In_ HRESULT hr);
#ifdef UNICODE
#define DXGetErrorString DXGetErrorStringW
#else
#define DXGetErrorString DXGetErrorStringA
#endif
	void WINAPI DXGetErrorDescriptionW(_In_ HRESULT hr, _Out_cap_(count) WCHAR* desc, _In_ size_t count);
	void WINAPI DXGetErrorDescriptionA(_In_ HRESULT hr, _Out_cap_(count) CHAR* desc, _In_ size_t count);
#ifdef UNICODE
#define DXGetErrorDescription DXGetErrorDescriptionW
#else
#define DXGetErrorDescription DXGetErrorDescriptionA
#endif
	HRESULT WINAPI DXTraceW(_In_z_ const WCHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR* strMsg, _In_ bool bPopMsgBox);
	HRESULT WINAPI DXTraceA(_In_z_ const CHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const CHAR* strMsg, _In_ bool bPopMsgBox);
#ifdef UNICODE
#define DXTrace DXTraceW
#else
#define DXTrace DXTraceA
#endif
#if defined(DEBUG) || defined(_DEBUG)
#ifdef UNICODE
#define DXTRACE_MSG(str)              DXTrace( __FILEW__, (DWORD)__LINE__, 0, str, false )
#define DXTRACE_ERR(str,hr)           DXTrace( __FILEW__, (DWORD)__LINE__, hr, str, false )
#define DXTRACE_ERR_MSGBOX(str,hr)    DXTrace( __FILEW__, (DWORD)__LINE__, hr, str, true )
#else
#define DXTRACE_MSG(str)              DXTrace( __FILE__, (DWORD)__LINE__, 0, str, false )
#define DXTRACE_ERR(str,hr)           DXTrace( __FILE__, (DWORD)__LINE__, hr, str, false )
#define DXTRACE_ERR_MSGBOX(str,hr)    DXTrace( __FILE__, (DWORD)__LINE__, hr, str, true )
#endif
#else
#define DXTRACE_MSG(str)              (0L)
#define DXTRACE_ERR(str,hr)           (hr)
#define DXTRACE_ERR_MSGBOX(str,hr)    (hr)
#endif

#ifdef __cplusplus
}
#endif

