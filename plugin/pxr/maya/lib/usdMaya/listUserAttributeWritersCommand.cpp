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

#include "usdMaya/listUserAttributeWritersCommand.h"

#include "usdMaya/userAttributeWriterRegistry.h"
#include "usdMaya/registryHelper.h"

#include <maya/MSyntax.h>
#include <maya/MStatus.h>
#include <maya/MGlobal.h>
#include <maya/MString.h>

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

usdListUserAttributeWriters::usdListUserAttributeWriters()
{

}

usdListUserAttributeWriters::~usdListUserAttributeWriters()
{

}

MStatus
usdListUserAttributeWriters::doIt(const MArgList& args)
{
    for (const auto& e : UsdMayaUserAttributeWriterRegistry::ListWriters()) {
        appendToResult(e.GetString().c_str());
    }

    return MS::kSuccess;
}

void* usdListUserAttributeWriters::creator()
{
    return new usdListUserAttributeWriters();
}

PXR_NAMESPACE_CLOSE_SCOPE
