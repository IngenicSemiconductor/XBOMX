/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_NDEBUG 0
#define LOG_TAG "HardOMXPlugin"
#include <utils/Log.h>

#include "HardOMXPlugin.h"

#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AString.h>

#include <dlfcn.h>

namespace android {

HardOMXPlugin::HardOMXPlugin() 
     :mLibHandle(dlopen("libOMX_Core.so", RTLD_NOW)),
      mInit(NULL),
      mDeinit(NULL),
      mComponentNameEnum(NULL),
      mGetHandle(NULL),
      mFreeHandle(NULL),
      mGetRolesOfComponentHandle(NULL) {
      if (mLibHandle != NULL) {
          mInit = (InitFunc)dlsym(mLibHandle, "OMX_Init");
	  mDeinit = (DeinitFunc)dlsym(mLibHandle, "OMX_Deinit");
	  
	  mComponentNameEnum =
              (ComponentNameEnumFunc)dlsym(mLibHandle, "OMX_ComponentNameEnum");
	  
	  mGetHandle = (GetHandleFunc)dlsym(mLibHandle, "OMX_GetHandle");
	  mFreeHandle = (FreeHandleFunc)dlsym(mLibHandle, "OMX_FreeHandle");
	  
	  mGetRolesOfComponentHandle =
              (GetRolesOfComponentFunc)dlsym(
                    mLibHandle, "OMX_GetRolesOfComponent");

	  (*mInit)();
      }
      else
          ALOGE("%s: failed to load %s", __func__, "libOMX_Core.so");
}

HardOMXPlugin::~HardOMXPlugin() {
    if (mLibHandle != NULL) {
        (*mDeinit)();

        dlclose(mLibHandle);
        mLibHandle = NULL;
    }
}

OMX_ERRORTYPE HardOMXPlugin::makeComponentInstance(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component) {
    ALOGV("111 makeComponentInstance '%s'", name);
    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    return (*mGetHandle)(
            reinterpret_cast<OMX_HANDLETYPE *>(component),
            const_cast<char *>(name),
            appData, const_cast<OMX_CALLBACKTYPE *>(callbacks));
}

OMX_ERRORTYPE HardOMXPlugin::destroyComponentInstance(
        OMX_COMPONENTTYPE *component) {
    if (mLibHandle == NULL) {
        return OMX_ErrorUndefined;
    }

    return (*mFreeHandle)(reinterpret_cast<OMX_HANDLETYPE *>(component));
}

OMX_ERRORTYPE HardOMXPlugin::enumerateComponents(
        OMX_STRING name,
        size_t size,
        OMX_U32 index) {
    if (mLibHandle == NULL) {
	ALOGE("mLibHandle is NULL!");
        return OMX_ErrorUndefined;
    }

    return (*mComponentNameEnum)(name, size, index);
}

OMX_ERRORTYPE HardOMXPlugin::getRolesOfComponent(
        const char *name,
        Vector<String8> *roles) {

    roles->clear();
    OMX_U32 numRoles = 1;
    OMX_U8 **array = new OMX_U8 *[numRoles];
    array[0] = new OMX_U8[OMX_MAX_STRINGNAME_SIZE];
    OMX_ERRORTYPE err =  (*mGetRolesOfComponentHandle)(
            const_cast<OMX_STRING>(name), &numRoles, array);
    if (err == OMX_ErrorNone) {
      String8 s((const char *)array[0]);
      roles->push(s);
    }
    return err;
}

OMXPluginBase * createOMXPlugin(){
    return new HardOMXPlugin();
}    
}  // namespace android
