/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file       Exynos_OMX_Core.c
 * @brief      Exynos OpenMAX IL Core
 * @author     SeungBeom Kim (sbcrux.kim@samsung.com)
 *             HyeYeon Chung (hyeon.chung@samsung.com)
 *             Yunji Kim (yunji.kim@samsung.com)
 * @version    2.0.0
 * @history
 *    2012.02.20 : Create
 */
#include <dlfcn.h>
#include <stdio.h>
#include <OMX_Core.h>
#include <OMX_Component.h>
#include <utils/Log.h>

#define MAXCOMP (50)
#define MAXNAMESIZE (128)

/** Determine the number of elements in an array */
#define COUNTOF(x) (sizeof(x)/sizeof(x[0]))

/** Array to hold the DLL pointers for each allocated component */
static void *pModules[MAXCOMP] = { 0 };

/** Array to hold the component handles for each allocated component */
static void *pComponents[COUNTOF(pModules)] = { 0 };

static const struct {
    const char *mName;
    const char *mLibNameSuffix;
    const char *mRole;

} kComponents[] = {
  { "OMX.LUMEVideoDecoder", "vlume", "video_decoder.avc" },
  { "OMX.LUMEVideoDecoder", "vlume", "video_decoder.mpeg4" },
  { "OMX.LUMEVideoDecoder", "vlume", "video_decoder.wmv3"},
  { "OMX.LUMEVideoDecoder", "vlume", "video_decoder.rv40"},
  { "OMX.LUMEAudioDecoder", "alume", "audio_decoder.aac" },  
  //{ "OMX.ingenic.h264.decoder", "vlume", "video_decoder.avc" },
  //{ "OMX.ingenic.mpeg4.decoder", "vlume", "video_decoder.mpeg4" },
  { "OMX.ingenic.x264.encoder", "x264hwenc", "video_encoder.avc" },
};

static const int kNumComponents =
    sizeof(kComponents) / sizeof(kComponents[0]);

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN  OMX_U32 nNameLength,
    OMX_IN  OMX_U32 nIndex)
{
    if (nIndex >= kNumComponents) {
        return OMX_ErrorNoMore;
    }

    strcpy(cComponentName, kComponents[nIndex].mName);

    return OMX_ErrorNone;

}

OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE * pHandle,
    OMX_STRING cComponentName, OMX_PTR pAppData,
    OMX_CALLBACKTYPE * pCallBacks)
{
    int i = 0;
    for (i = 0; i < kNumComponents; ++i) {
        if (strcmp(cComponentName, kComponents[i].mName)) {
            continue;
        }
	static const char prefix[] = "libstagefright_hard_";
	static const char postfix[] = ".so";
	char libName[sizeof(prefix) + MAXNAMESIZE + sizeof(postfix)];
	strcpy(libName, prefix);
	strcat(libName, kComponents[i].mLibNameSuffix);
	strcat(libName, postfix);

        void *libHandle = dlopen(libName, RTLD_NOW);

        if (libHandle == NULL) {
            ALOGE("unable to dlopen %s", libName);

            return OMX_ErrorComponentNotFound;
        }

        typedef OMX_ERRORTYPE (*CreateHardOMXComponentFunc)(
                const char *, const OMX_CALLBACKTYPE *,
                OMX_PTR, OMX_COMPONENTTYPE **);

        CreateHardOMXComponentFunc createHardOMXComponent =
            (CreateHardOMXComponentFunc)dlsym(
                    libHandle,
                    "_Z22createHardOMXComponentPKcPK16OMX_CALLBACKTYPE"
                    "PvPP17OMX_COMPONENTTYPE");

        if (createHardOMXComponent == NULL) {
            dlclose(libHandle);
            libHandle = NULL;
	    ALOGE("createHardOMXComponent == NULL ...");
            return OMX_ErrorComponentNotFound;
        }

        OMX_ERRORTYPE err =
	  (*createHardOMXComponent)(cComponentName, pCallBacks, pAppData, pHandle);

        if (err != OMX_ErrorNone) {
            dlclose(libHandle);
            libHandle = NULL;

            return err;
        }

	/* Locate the first empty slot for a component.  If no slots
	 * are available, error out */
	for (i = 0; i < COUNTOF(pModules); i++)
	{
	    if (pModules[i] == NULL)
	        break;
	}

	pModules[i] = libHandle;
	pComponents[i] = *pHandle;
        return OMX_ErrorNone;
    }

    return OMX_ErrorInvalidComponentName;
}

OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent)
{    
    int i = 0;
    /* Locate the component handle in the array of handles */
    for (i = 0; i < COUNTOF(pModules); i++)
    {
	if (pComponents[i] == hComponent)
	    break;
    }

    /* release the component and the component handle */
    ((OMX_COMPONENTTYPE *)hComponent)->ComponentDeInit(hComponent);
    dlclose(pModules[i]);
    pModules[i] = NULL;    
    pComponents[i] = NULL;

    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_SetupTunnel(
    OMX_IN OMX_HANDLETYPE hOutput,
    OMX_IN OMX_U32 nPortOutput,
    OMX_IN OMX_HANDLETYPE hInput,
    OMX_IN OMX_U32 nPortInput)
{
    OMX_ERRORTYPE ret = OMX_ErrorNotImplemented;

EXIT:
    return ret;
}

OMX_API OMX_ERRORTYPE OMX_GetContentPipe(
    OMX_OUT OMX_HANDLETYPE *hPipe,
    OMX_IN  OMX_STRING szURI)
{
    OMX_ERRORTYPE ret = OMX_ErrorNotImplemented;

EXIT:
    return ret;
}

OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole (
    OMX_IN    OMX_STRING role,
    OMX_INOUT OMX_U32 *pNumComps,
    OMX_INOUT OMX_U8  **compNames)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;
    int i = 0;

    *pNumComps = 0;

    for (i = 0; i < kNumComponents; i++) {
	if (strcmp(kComponents[i].mRole, role) == 0) {
	  if (compNames != NULL) {
	    strcpy((OMX_STRING)compNames[*pNumComps], kComponents[i].mName);
	  }
	  *pNumComps = (*pNumComps + 1);
	}
    }

    return ret;
}

OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent (
    OMX_IN    OMX_STRING compName,
    OMX_INOUT OMX_U32 *pNumRoles,
    OMX_OUT   OMX_U8 **roles)
{
    int i = 0;
    for ( i = 0; i < kNumComponents; ++i) {
        if (strcmp(compName, kComponents[i].mName)) {
            continue;
        }
	strcpy((OMX_STRING) roles[0], kComponents[i].mRole);
        return OMX_ErrorNone;
    }

    return OMX_ErrorInvalidComponentName;
}
