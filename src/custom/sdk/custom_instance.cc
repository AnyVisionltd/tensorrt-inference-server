// Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of NVIDIA CORPORATION nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "custom_instance.h"

namespace nvidia { namespace inferenceserver { namespace custom {

CustomInstance::CustomInstance(
    const std::string& instance_name, const ModelConfig& model_config)
    : instance_name_(instance_name), model_config_(model_config)
{
}

int
CustomInstance::Init()
{
  // Add common initailization here

  return InitContext();
}

int
CustomInstance::Execute(
    const uint32_t payload_cnt, CustomPayload* payloads,
    CustomGetNextInputFn_t input_fn, CustomGetOutputFn_t output_fn)
{
  for (uint32_t pidx = 0; pidx < payload_cnt; ++pidx) {
    ExecutePayload(payloads[pidx], input_fn, output_fn);
  }

  return kSuccess;
}

const char*
CustomInstance::ErrorString(int errcode)
{
  if (errcode >= kNumErrorCodes) {
    return CustomErrorString(errcode);
  }

  switch (errcode) {
    case kSuccess:
      return "success";
    case kInvalidModelConfig:
      return "invalid model configuration";
    default:
      break;
  }

  return "unknown error";
}

const char*
CustomInstance::CustomErrorString(int errcode)
{
  return "unknown error";
}

/////////////

extern "C" {

int
CustomInitialize(const CustomInitializeData* data, void** custom_instance)
{
  // Convert the serialized model config to a ModelConfig object.
  ModelConfig model_config;
  if (!model_config.ParseFromString(std::string(
          data->serialized_model_config, data->serialized_model_config_size))) {
    return kInvalidModelConfig;
  }

  // Create the instance and validate that the model configuration is
  // something that we can handle.
  CustomInstance* instance = CustomInstance::Create(data, model_config);
  // new CustomInstance(
  //     std::string(data->instance_name), model_config, data->gpu_device_id);
  int err = instance->Init();
  if (err != kSuccess) {
    return err;
  }

  *custom_instance = static_cast<void*>(instance);

  return kSuccess;
}

int
CustomFinalize(void* custom_instance)
{
  if (custom_instance != nullptr) {
    CustomInstance* instance = static_cast<CustomInstance*>(custom_instance);
    delete instance;
  }

  return kSuccess;
}

const char*
CustomErrorString(void* custom_instance, int errcode)
{
  CustomInstance* instance = static_cast<CustomInstance*>(custom_instance);

  return instance->ErrorString(errcode);
}

int
CustomExecute(
    void* custom_instance, const uint32_t payload_cnt, CustomPayload* payloads,
    CustomGetNextInputFn_t input_fn, CustomGetOutputFn_t output_fn)
{
  if (custom_instance == nullptr) {
    return kUnknown;
  }

  CustomInstance* instance = static_cast<CustomInstance*>(custom_instance);
  return instance->Execute(payload_cnt, payloads, input_fn, output_fn);
}

}  // extern "C"

}}}  // namespace nvidia::inferenceserver::custom