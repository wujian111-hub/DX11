#pragma once

#ifndef GRAPHICS_THROW_MACROS_H
#define GRAPHICS_THROW_MACROS_H

#include "Graphics.h"

#define GFX_EXCEPT_NOINFO(hr) GraphicsHrException( __LINE__,__FILE__,(hr) )
#define GFX_THROW_NOINFO(hrcall) if( FAILED( hr = (hrcall) ) ) throw GraphicsHrException( __LINE__,__FILE__,hr )

#ifndef NDEBUG
#define GFX_EXCEPT(hr) GraphicsHrException( __LINE__,__FILE__,(hr),infoManager.GetMessages() )

// 先取消定义，避免重定义错误
#ifdef GFX_THROW_INFO
#undef GFX_THROW_INFO
#endif
#define GFX_THROW_INFO(hrcall) infoManager.Set(); if( FAILED( hr = (hrcall) ) ) throw GFX_EXCEPT(hr)

#define GFX_DEVICE_REMOVED_EXCEPT(hr) GraphicsDeviceRemovedException( __LINE__,__FILE__,(hr),infoManager.GetMessages() )
#define GFX_THROW_INFO_ONLY(call) infoManager.Set(); (call); {auto v = infoManager.GetMessages(); if(!v.empty()) {throw GraphicsInfoException( __LINE__,__FILE__,v);}}
#else
#define GFX_EXCEPT(hr) GraphicsHrException( __LINE__,__FILE__,(hr) )

#ifdef GFX_THROW_INFO
#undef GFX_THROW_INFO
#endif
#define GFX_THROW_INFO(hrcall) GFX_THROW_NOINFO(hrcall)

#define GFX_DEVICE_REMOVED_EXCEPT(hr) GraphicsDeviceRemovedException( __LINE__,__FILE__,(hr) )
#define GFX_THROW_INFO_ONLY(call) (call)
#endif

// INFOMAN macro is now defined in Graphics.h

#ifdef NDEBUG
#define INFOMAN_NOHR(gfx)
#else
#define INFOMAN_NOHR(gfx) DxgiInfoManager& infoManager = GetInfoManager((gfx))
#endif

#endif