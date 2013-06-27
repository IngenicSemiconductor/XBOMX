/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef HARDWARE_RENDERER_H_

#define HARDWARE_RENDERER_H_

#include <utils/RefBase.h>
#include <ui/ANativeObjectBase.h>

#include <media/IMediaPlayerService.h>


namespace android {

struct MetaData;

typedef struct RenderData{
  void* input;
  size_t inputSize;
  void* platformPrivate;
  //VideoWindowState* state;
  bool needReinit;
  buffer_handle_t bufferHandle;
} RenderData;

class HardwareRenderer : public RefBase{
public:
    HardwareRenderer(){}

    virtual void render(RenderData* data) = 0;
protected:
    virtual ~HardwareRenderer(){}

private:
    HardwareRenderer(const HardwareRenderer &);
    HardwareRenderer &operator=(const HardwareRenderer &);
};

}  // namespace android

#endif  // HARDWARE_RENDERER_H_
