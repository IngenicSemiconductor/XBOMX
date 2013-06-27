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
#define LOG_TAG "HardOMXComponent"
#include <utils/Log.h>

#include "HardOMXComponent.h"

#include <media/stagefright/foundation/ADebug.h>

namespace android {

HardOMXComponent::HardOMXComponent(
        const char *name,
        const OMX_CALLBACKTYPE *callbacks,
        OMX_PTR appData,
        OMX_COMPONENTTYPE **component)
    : mName(name),
      mCallbacks(callbacks),
      mComponent(new OMX_COMPONENTTYPE),
      mLibHandle(NULL) {
    mComponent->nSize = sizeof(*mComponent);
    mComponent->nVersion.s.nVersionMajor = 1;
    mComponent->nVersion.s.nVersionMinor = 0;
    mComponent->nVersion.s.nRevision = 0;
    mComponent->nVersion.s.nStep = 0;
    mComponent->pComponentPrivate = this;
    mComponent->pApplicationPrivate = appData;

    mComponent->GetComponentVersion = NULL;
    mComponent->SendCommand = SendCommandWrapper;
    mComponent->GetParameter = GetParameterWrapper;
    mComponent->SetParameter = SetParameterWrapper;
    mComponent->GetConfig = GetConfigWrapper;
    mComponent->SetConfig = SetConfigWrapper;
    mComponent->GetExtensionIndex = GetExtensionIndexWrapper;
    mComponent->GetState = GetStateWrapper;
    mComponent->ComponentTunnelRequest = NULL;
    mComponent->UseBuffer = UseBufferWrapper;
    mComponent->AllocateBuffer = AllocateBufferWrapper;
    mComponent->FreeBuffer = FreeBufferWrapper;
    mComponent->EmptyThisBuffer = EmptyThisBufferWrapper;
    mComponent->FillThisBuffer = FillThisBufferWrapper;
    mComponent->SetCallbacks = NULL;
    mComponent->ComponentDeInit = ComponentDeInitWrapper;
    mComponent->UseEGLImage = NULL;
    mComponent->ComponentRoleEnum = NULL;
    ALOGV("HardOMXComponent::HardOMXComponent construction ....");
    *component = mComponent;
}

HardOMXComponent::~HardOMXComponent() {
    delete mComponent;
    mComponent = NULL;
}

void HardOMXComponent::setLibHandle(void *libHandle) {
    CHECK(libHandle != NULL);
    mLibHandle = libHandle;
}

void *HardOMXComponent::libHandle() const {
    return mLibHandle;
}

OMX_ERRORTYPE HardOMXComponent::initCheck() const {
    return OMX_ErrorNone;
}

const char *HardOMXComponent::name() const {
    return mName.c_str();
}

void HardOMXComponent::notify(
        OMX_EVENTTYPE event,
        OMX_U32 data1, OMX_U32 data2, OMX_PTR data) {
    (*mCallbacks->EventHandler)(
            mComponent,
            mComponent->pApplicationPrivate,
            event,
            data1,
            data2,
            data);
}

void HardOMXComponent::notifyEmptyBufferDone(OMX_BUFFERHEADERTYPE *header) {
    (*mCallbacks->EmptyBufferDone)(
            mComponent, mComponent->pApplicationPrivate, header);
}

void HardOMXComponent::notifyFillBufferDone(OMX_BUFFERHEADERTYPE *header) {
    (*mCallbacks->FillBufferDone)(
            mComponent, mComponent->pApplicationPrivate, header);
}

// static
OMX_ERRORTYPE HardOMXComponent::SendCommandWrapper(
        OMX_HANDLETYPE component,
        OMX_COMMANDTYPE cmd,
        OMX_U32 param,
        OMX_PTR data) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->sendCommand(cmd, param, data);
}

// static
OMX_ERRORTYPE HardOMXComponent::GetParameterWrapper(
        OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR params) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getParameter(index, params);
}

// static
OMX_ERRORTYPE HardOMXComponent::SetParameterWrapper(
        OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR params) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    //ALOGE("HardOMXComponent::SetParameterWrapper11111");
    return me->setParameter(index, params);
}

// static
OMX_ERRORTYPE HardOMXComponent::GetConfigWrapper(
        OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR params) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getConfig(index, params);
}

// static
OMX_ERRORTYPE HardOMXComponent::SetConfigWrapper(
        OMX_HANDLETYPE component,
        OMX_INDEXTYPE index,
        OMX_PTR params) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->setConfig(index, params);
}

// static
OMX_ERRORTYPE HardOMXComponent::GetExtensionIndexWrapper(
        OMX_HANDLETYPE component,
        OMX_STRING name,
        OMX_INDEXTYPE *index) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;
    OMX_ERRORTYPE           ret = OMX_ErrorNone;
    if (strcmp(name, "OMX.google.android.index.enableAndroidNativeBuffers") == 0){
      //      *index = (OMX_INDEXTYPE) OMX_IndexParamEnableAndroidBuffers;
      *index = (OMX_INDEXTYPE)0x7F000011;
    }else if (strcmp(name, "OMX.google.android.index.getAndroidNativeBufferUsage")== 0){
      *index = (OMX_INDEXTYPE)0x7F000012;
    }
    else if (strcmp(name, "OMX.google.android.index.useAndroidNativeBuffer2")== 0){
       //*index = (OMX_INDEXTYPE)0x7F000013;
    // else if (strcmp(name, EXYNOS_INDEX_PARAM_GET_ANB) == 0)
    //  *index = (OMX_INDEXTYPE) OMX_IndexParamGetAndroidNativeBuffer;
    //else if (strcmp(name, EXYNOS_INDEX_PARAM_USE_ANB) == 0)
    // *index = (OMX_INDEXTYPE) OMX_IndexParamUseAndroidNativeBuffer;
    }else if (strcmp(name, "OMX.lume.android.index.setShContext") == 0) {
      *index = (OMX_INDEXTYPE)0x7F000014;
    }else{
       ALOGE("411111111111111111111111111SOFT");
      return me->getExtensionIndex(name, index);
    }
    return ret;
    //return me->getExtensionIndex(name, index);
}

// static
OMX_ERRORTYPE HardOMXComponent::UseBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size,
        OMX_U8 *ptr) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->useBuffer(buffer, portIndex, appPrivate, size, ptr);
}

// static
OMX_ERRORTYPE HardOMXComponent::AllocateBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->allocateBuffer(buffer, portIndex, appPrivate, size);
}

// static
OMX_ERRORTYPE HardOMXComponent::FreeBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_U32 portIndex,
        OMX_BUFFERHEADERTYPE *buffer) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->freeBuffer(portIndex, buffer);
}

// static
OMX_ERRORTYPE HardOMXComponent::EmptyThisBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE *buffer) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->emptyThisBuffer(buffer);
}

// static
OMX_ERRORTYPE HardOMXComponent::FillThisBufferWrapper(
        OMX_HANDLETYPE component,
        OMX_BUFFERHEADERTYPE *buffer) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->fillThisBuffer(buffer);
}

// static
OMX_ERRORTYPE HardOMXComponent::GetStateWrapper(
        OMX_HANDLETYPE component,
        OMX_STATETYPE *state) {
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)component)->pComponentPrivate;

    return me->getState(state);
}

// static
OMX_ERRORTYPE HardOMXComponent::ComponentDeInitWrapper(OMX_HANDLETYPE hComponent) {
    ALOGV("ComponentDeInitWrapper in");
    HardOMXComponent *me =
        (HardOMXComponent *)
            ((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate;

    me->prepareForDestruction();

    CHECK_EQ(me->getStrongCount(), 1);
    me->decStrong(NULL);
    me = NULL;

    return OMX_ErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

OMX_ERRORTYPE HardOMXComponent::sendCommand(
        OMX_COMMANDTYPE cmd, OMX_U32 param, OMX_PTR data) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::getParameter(
        OMX_INDEXTYPE index, OMX_PTR params) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::setParameter(
        OMX_INDEXTYPE index, const OMX_PTR params) {

  ALOGE("HardOMXComponent::setParameter 2222");

    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::getConfig(
        OMX_INDEXTYPE index, OMX_PTR params) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::setConfig(
        OMX_INDEXTYPE index, const OMX_PTR params) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::getExtensionIndex(
        const char *name, OMX_INDEXTYPE *index) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::useBuffer(
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size,
        OMX_U8 *ptr) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::allocateBuffer(
        OMX_BUFFERHEADERTYPE **buffer,
        OMX_U32 portIndex,
        OMX_PTR appPrivate,
        OMX_U32 size) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::freeBuffer(
        OMX_U32 portIndex,
        OMX_BUFFERHEADERTYPE *buffer) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::emptyThisBuffer(
        OMX_BUFFERHEADERTYPE *buffer) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::fillThisBuffer(
        OMX_BUFFERHEADERTYPE *buffer) {
    return OMX_ErrorUndefined;
}

OMX_ERRORTYPE HardOMXComponent::getState(OMX_STATETYPE *state) {
    return OMX_ErrorUndefined;
}

}  // namespace android
