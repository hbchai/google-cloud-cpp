// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef GOOGLE_CLOUD_CPP_GENERATOR_INTERNAL_SERVICE_CODE_GENERATOR_H
#define GOOGLE_CLOUD_CPP_GENERATOR_INTERNAL_SERVICE_CODE_GENERATOR_H

#include "google/cloud/status.h"
#include "generator/internal/codegen_utils.h"
#include "generator/internal/generator_interface.h"
#include "generator/internal/printer.h"
#include <google/protobuf/descriptor.h>

namespace google {
namespace cloud {
namespace generator_internal {

class ServiceCodeGenerator : public GeneratorInterface {
 public:
  ServiceCodeGenerator(
      std::string const& header_path_key, std::string const& cc_path_key,
      google::protobuf::ServiceDescriptor const* service_descriptor,
      VarsDictionary service_vars,
      std::map<std::string, VarsDictionary> service_method_vars,
      google::protobuf::compiler::GeneratorContext* context);

  ~ServiceCodeGenerator() override = default;

  ServiceCodeGenerator(ServiceCodeGenerator const&) = delete;
  ServiceCodeGenerator& operator=(ServiceCodeGenerator const&) = delete;
  ServiceCodeGenerator(ServiceCodeGenerator&&) = default;
  ServiceCodeGenerator& operator=(ServiceCodeGenerator&&) = default;

  Status Generate() override;

 protected:
  virtual Status GenerateHeader() = 0;
  virtual Status GenerateCc() = 0;

  void SetVars(absl::string_view header_path);
  VarsDictionary MergeServiceAndMethodVars(
      google::protobuf::MethodDescriptor const& method) const;
  enum class FileType { kHeaderFile, kCcFile };
  static void GenerateLocalIncludes(Printer& p,
                                    std::vector<std::string> local_includes,
                                    FileType file_type = FileType::kHeaderFile);
  static void GenerateSystemIncludes(Printer& p,
                                     std::vector<std::string> system_includes);
  Status OpenNamespaces(Printer& p,
                        NamespaceType ns_type = NamespaceType::kNormal);
  void CloseNamespaces(Printer& p);

  google::protobuf::ServiceDescriptor const* service_descriptor_;
  VarsDictionary service_vars_;
  std::map<std::string, VarsDictionary> service_method_vars_;
  std::vector<std::string> namespaces_;
  Printer header_;
  Printer cc_;
};

}  // namespace generator_internal
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_GENERATOR_INTERNAL_SERVICE_CODE_GENERATOR_H
