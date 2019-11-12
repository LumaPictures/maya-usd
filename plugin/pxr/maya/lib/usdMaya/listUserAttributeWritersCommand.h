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
#ifndef PXRUSDMAYA_USD_LIST_USER_ATTRIBUTE_WRITERS_H
#define PXRUSDMAYA_USD_LIST_USER_ATTRIBUTE_WRITERS_H

#include "pxr/pxr.h"
#include "usdMaya/api.h"
#include <maya/MPxCommand.h>

PXR_NAMESPACE_OPEN_SCOPE

class usdListUserAttributeWriters : public MPxCommand
{
public:
    PXRUSDMAYA_API
    usdListUserAttributeWriters();
    PXRUSDMAYA_API
    virtual ~usdListUserAttributeWriters();

    PXRUSDMAYA_API
    virtual MStatus doIt(const MArgList& args);
    virtual bool  isUndoable () const { return false; };

    PXRUSDMAYA_API
    static void* creator();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXRUSDMAYA_USD_LIST_SHADING_MODES_H
