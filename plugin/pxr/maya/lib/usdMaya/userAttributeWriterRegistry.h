//
// Copyright 2019 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef PXRUSDMAYA_USERATTRIBUTEWRITERREGISTRY_H
#define PXRUSDMAYA_USERATTRIBUTEWRITERREGISTRY_H

#include "pxr/pxr.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/usd/usd/prim.h"

#include "usdMaya/api.h"

#include <maya/MPlug.h>

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_PTRS(UsdMayaUserAttributeWriterRegistry);

/// \class UsdMayaUserAttributeWriterRegistry
/// \brief This class provides access to the registered
/// userAttributeWriters.
class UsdMayaUserAttributeWriterRegistry : public TfWeakBase {
public:
    using UserAttributeWriter = std::function<UsdAttribute(
        const MPlug&,
        const UsdPrim&,
        const std::string&,
        const std::string&,
        const bool)>;

    PXRUSDMAYA_API
    static UsdMayaUserAttributeWriterRegistry& GetInstance();

    /// \brief registers a new user attribute writer.
    PXRUSDMAYA_API
    static void RegisterWriter(
        const std::string& name,
        const UserAttributeWriter& fn);

    /// \brief returns a list of available writers.
    static TfTokenVector ListWriters() {
        return GetInstance()._ListWriters();
    }

    /// \brief returns a writer registered to \p name or
    /// or a nullptr std::function of none available.
    static UserAttributeWriter GetWriter(const TfToken& name) {
        return GetInstance()._GetWriter(name);
    }
private:
    TfTokenVector _ListWriters();
    UserAttributeWriter _GetWriter(const TfToken& name);

    UsdMayaUserAttributeWriterRegistry();
    ~UsdMayaUserAttributeWriterRegistry();
    friend class TfSingleton<UsdMayaUserAttributeWriterRegistry>;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXRUSDMAYA_USERATTRIBUTEWRITERREGISTRY_H
