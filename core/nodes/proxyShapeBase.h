//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXRUSDMAYA_PROXY_SHAPE_BASE_H
#define PXRUSDMAYA_PROXY_SHAPE_BASE_H

/// \file usdMaya/proxyShapeBase.h

#include "../base/api.h"
#include "../listeners/stageNoticeListener.h"
#include "usdPrimProvider.h"

#include "pxr/pxr.h"

#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/notice.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/timeCode.h"

#include <maya/MBoundingBox.h>
#include <maya/MDagPath.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MDGContext.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MPxSurfaceShape.h>
#include <maya/MSelectionMask.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#include <map>


PXR_NAMESPACE_OPEN_SCOPE


#define MAYAUSD_PROXY_SHAPE_BASE_TOKENS \
    ((MayaTypeName, "mayaUsdProxyShapeBase"))

TF_DECLARE_PUBLIC_TOKENS(MayaUsdProxyShapeBaseTokens,
                         MAYAUSD_CORE_PUBLIC,
                         MAYAUSD_PROXY_SHAPE_BASE_TOKENS);


class MayaUsdProxyShapeBase : public MPxSurfaceShape,
                              public UsdMayaUsdPrimProvider
{
    public:
        typedef MayaUsdProxyShapeBase ThisClass;

        MAYAUSD_CORE_PUBLIC
        static const MTypeId typeId;
        MAYAUSD_CORE_PUBLIC
        static const MString typeName;

        MAYAUSD_CORE_PUBLIC
        static const MString displayFilterName;
        MAYAUSD_CORE_PUBLIC
        static const MString displayFilterLabel;

        // Attributes
        MAYAUSD_CORE_PUBLIC
        static MObject filePathAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject primPathAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject excludePrimPathsAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject timeAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject complexityAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject inStageDataAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject inStageDataCachedAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject outStageDataAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject drawRenderPurposeAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject drawProxyPurposeAttr;
        MAYAUSD_CORE_PUBLIC
        static MObject drawGuidePurposeAttr;

        /// Delegate function for computing the closest point and surface normal
        /// on the proxy shape to a given ray.
        /// The input ray, output point, and output normal should be in the
        /// proxy shape's local space.
        /// Should return true if a point was found, and false otherwise.
        /// (You could just treat this as a ray intersection and return true
        /// if intersected, false if missed.)
        typedef std::function<bool(const MayaUsdProxyShapeBase&, const GfRay&,
                GfVec3d*, GfVec3d*)> ClosestPointDelegate;

        MAYAUSD_CORE_PUBLIC
        static void* creator();

        MAYAUSD_CORE_PUBLIC
        static MStatus initialize();

        MAYAUSD_CORE_PUBLIC
        static MayaUsdProxyShapeBase* GetShapeAtDagPath(const MDagPath& dagPath);

        MAYAUSD_CORE_PUBLIC
        static void SetClosestPointDelegate(ClosestPointDelegate delegate);

        // UsdMayaUsdPrimProvider overrides:
        /**
         * accessor to get the usdprim
         *
         * This method pulls the usdstage data from outData, and will evaluate
         * the dependencies necessary to do so. It should be called instead of
         * pulling on the data directly.
         */
        MAYAUSD_CORE_PUBLIC
        UsdPrim usdPrim() const override;

        // Virtual function overrides

        // A ProxyShapeBase node cannot be created directly; it only exists
        // as a base class.
        MAYAUSD_CORE_PUBLIC
        bool isAbstractClass() const override;
        MAYAUSD_CORE_PUBLIC
        void postConstructor() override;
        MAYAUSD_CORE_PUBLIC
        MStatus compute(
                const MPlug& plug,
                MDataBlock& dataBlock) override;
        MAYAUSD_CORE_PUBLIC
        bool isBounded() const override;
        MAYAUSD_CORE_PUBLIC
        MBoundingBox boundingBox() const override;
        MAYAUSD_CORE_PUBLIC
        MSelectionMask getShapeSelectionMask() const override;

        MAYAUSD_CORE_PUBLIC
        bool closestPoint(
                const MPoint& raySource,
                const MVector& rayDirection,
                MPoint& theClosestPoint,
                MVector& theClosestNormal,
                bool findClosestOnMiss,
                double tolerance) override;

        MAYAUSD_CORE_PUBLIC
        bool canMakeLive() const override;

        // Public functions
        MAYAUSD_CORE_PUBLIC
        virtual SdfPathVector getExcludePrimPaths() const;

        MAYAUSD_CORE_PUBLIC
        int getComplexity() const;
        MAYAUSD_CORE_PUBLIC
        UsdTimeCode getTime() const;

        MAYAUSD_CORE_PUBLIC
        bool GetAllRenderAttributes(
                UsdPrim* usdPrimOut,
                SdfPathVector* excludePrimPathsOut,
                int* complexityOut,
                UsdTimeCode* timeOut,
                bool* drawRenderPurpose,
                bool* drawProxyPurpose,
                bool* drawGuidePurpose);

        MAYAUSD_CORE_PUBLIC
        MStatus setDependentsDirty(
                const MPlug& plug,
                MPlugArray& plugArray) override;

        /// \brief  Clears the bounding box cache of the shape
        MAYAUSD_CORE_PUBLIC
        void clearBoundingBoxCache();

    protected:
        MAYAUSD_CORE_PUBLIC
        MayaUsdProxyShapeBase();

        MAYAUSD_CORE_PUBLIC
        ~MayaUsdProxyShapeBase() override;

        MAYAUSD_CORE_PUBLIC
        bool isStageValid() const;

        // Hook method for derived classes.  This class returns a nullptr.
        MAYAUSD_CORE_PUBLIC
        virtual SdfLayerRefPtr computeSessionLayer(MDataBlock&);

        // Hook method for derived classes: can this object be soft selected?
        // This class returns false.
        MAYAUSD_CORE_PUBLIC
        virtual bool canBeSoftSelected() const;

        // Hook method for derived classes: is soft select enabled?  This
        // class returns false.
        MAYAUSD_CORE_PUBLIC
        virtual bool GetObjectSoftSelectEnabled() const;

        MAYAUSD_CORE_PUBLIC
        UsdPrim _GetUsdPrim(MDataBlock dataBlock) const;

        // Hook method for derived classes: cache an empty computed bounding
        // box.  This class does nothing.
        MAYAUSD_CORE_PUBLIC
        virtual void CacheEmptyBoundingBox(MBoundingBox&);

        // Return the output time.  This class returns the value of the 
        // input time attribute.
        MAYAUSD_CORE_PUBLIC
        virtual UsdTimeCode GetOutputTime(MDataBlock) const;

    private:
        MayaUsdProxyShapeBase(const MayaUsdProxyShapeBase&);
        MayaUsdProxyShapeBase& operator=(const MayaUsdProxyShapeBase&);

        MStatus computeInStageDataCached(MDataBlock& dataBlock);
        MStatus computeOutStageData(MDataBlock& dataBlock);

        SdfPathVector _GetExcludePrimPaths(MDataBlock dataBlock) const;
        int _GetComplexity(MDataBlock dataBlock) const;
        UsdTimeCode _GetTime(MDataBlock dataBlock) const;

        bool _GetDrawPurposeToggles(
                MDataBlock dataBlock,
                bool* drawRenderPurpose,
                bool* drawProxyPurpose,
                bool* drawGuidePurpose) const;

        void _OnStageContentsChanged(
                const UsdNotice::StageContentsChanged& notice);

        UsdMayaStageNoticeListener _stageNoticeListener;

        std::map<UsdTimeCode, MBoundingBox> _boundingBoxCache;

        static ClosestPointDelegate _sharedClosestPointDelegate;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
