//
// Copyright 2019 Luma Pictures
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
#include "materialAdapter.h"

#include <pxr/base/tf/fileUtils.h>

#include <pxr/imaging/hd/instanceRegistry.h>
#include <pxr/imaging/hd/material.h>
#include <pxr/imaging/hd/resourceRegistry.h>

#if USD_VERSION_NUM >= 1905
#include <pxr/imaging/hio/glslfx.h>
#else
#include <pxr/imaging/glf/glslfx.h>
typedef PXR_NS::GlfGLSLFX HioGlslfx;
#endif // USD_VERSION_NUM >= 1905

#include <pxr/imaging/glf/textureRegistry.h>

#include <pxr/usdImaging/usdImaging/tokens.h>
#include <pxr/usdImaging/usdImagingGL/package.h>

#include <pxr/imaging/hdSt/textureResource.h>

#include <pxr/usd/sdf/types.h>

#include <maya/MNodeMessage.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MRenderUtil.h>

#include "adapterRegistry.h"
#include "mayaAttrs.h"
#include "tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

#if USD_VERSION_NUM <= 1911

const char* _simpleTexturedSurfaceSource =
    R"SURFACE(-- glslfx version 0.1

#import $TOOLS/glf/shaders/simpleLighting.glslfx

-- configuration
{
    "techniques": {
        "default": {
            "surfaceShader": {
                "source": [ "simpleTexturedSurface.Surface" ]
            }
        }
    }
}

-- glsl simpleTexturedSurface.Surface

vec4 surfaceShader(vec4 Peye, vec3 Neye, vec4 color, vec4 patchCoord)
{
#if defined(HD_HAS_color)
    return HdGet_color();
#else
    return vec4(1.0, 0.0, 0.0, 1.0);
#endif
})SURFACE";

static const std::pair<std::string, std::string> _textureShaderSource =
    []() -> std::pair<std::string, std::string> {
    std::istringstream ss(_simpleTexturedSurfaceSource);
    HioGlslfx gfx(ss);
    return {gfx.GetSurfaceSource(), gfx.GetDisplacementSource()};
}();

#endif // USD_VERSION_NUM <= 1911

const TfTokenVector _stSamplerCoords = {HdMayaAdapterTokens->st};

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (imagePlaneStReader)
    (imagePlaneTexture)
    (colorOpacity)
    (varname)
);

const MString _imageName("imageName");

} // namespace

class HdMayaImagePlaneMaterialAdapter : public HdMayaMaterialAdapter {
public:
    HdMayaImagePlaneMaterialAdapter(
        const SdfPath& id, HdMayaDelegateCtx* delegate, const MObject& obj)
        : HdMayaMaterialAdapter(id, delegate, obj) {}

    void CreateCallbacks() override {
        TF_DEBUG(HDMAYA_ADAPTER_CALLBACKS)
            .Msg(
                "Creating image plane material adapter callbacks for prim "
                "(%s).\n",
                GetID().GetText());

        MStatus status;
        auto obj = GetNode();
        auto id = MNodeMessage::addNodeDirtyPlugCallback(
            obj, _DirtyMaterialParams, this, &status);
        if (ARCH_LIKELY(status)) { AddCallback(id); }
        HdMayaAdapter::CreateCallbacks();
    }

#if USD_VERSION_NUM <= 1911

    std::string GetSurfaceShaderSource() override {
        return _textureShaderSource.first;
    }

    std::string GetDisplacementShaderSource() override {
        return _textureShaderSource.second;
    }

    inline bool _RegisterTexture(
        const MFnDependencyNode& node, const TfToken& paramName) {
        const auto filePath = _GetTextureFilePath(node);
        auto textureId = _GetTextureResourceID(filePath);
        if (textureId != HdTextureResource::ID(-1)) {
            HdResourceRegistry::TextureKey textureKey =
                GetDelegate()->GetRenderIndex().GetTextureKey(textureId);
            const auto& resourceRegistry =
                GetDelegate()->GetRenderIndex().GetResourceRegistry();
            HdInstance<
                HdResourceRegistry::TextureKey, HdTextureResourceSharedPtr>
                textureInstance;
            auto regLock = resourceRegistry->RegisterTextureResource(
                textureKey, &textureInstance);
            if (textureInstance.IsFirstInstance()) {
                auto textureResource = _GetTextureResource(filePath);
                _textureResources[paramName] = textureResource;
                textureInstance.SetValue(textureResource);
            } else {
                _textureResources[paramName] = textureInstance.GetValue();
            }
            return true;
        } else {
            _textureResources[paramName].reset();
        }
        return false;
    }

    HdMaterialParamVector GetMaterialParams() override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg("HdMayaImagePlaneMaterialAdapter::GetMaterialParams()\n");
        MStatus status;
        MFnDependencyNode node(_node, &status);
        if (ARCH_UNLIKELY(!status)) { return {}; }

        if (_RegisterTexture(node, HdMayaAdapterTokens->color)) {
            HdMaterialParam color(
                HdMaterialParam::ParamTypeTexture, HdMayaAdapterTokens->color,
                VtValue(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f)),
                GetID().AppendProperty(HdMayaAdapterTokens->color), _stSamplerCoords);
            return {color};
        }
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg("Unexpected failure to register texture\n");
        return {};
    }

    VtValue GetMaterialParamValue(const TfToken& paramName) override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg("Unexpected call to GetMaterialParamValue\n");
        return VtValue(GfVec4f(0.0f, 0.0f, 0.0f, 1.0f));
    }

#else // USD_VERSION_NUM > 1911

    VtValue GetMaterialResource() override {
        HdMaterialNetworkMap map;
        HdMaterialNetwork network;
        HdMaterialNode imagePlaneMat;
        auto& materialID = GetID();
        imagePlaneMat.path = materialID;
        imagePlaneMat.identifier = UsdImagingTokens->UsdImagePlaneSurface;

        auto texturePath = _GetTextureFilePath(MFnDependencyNode(_node));
        if (!texturePath.IsEmpty()) {

            // TODO: query sdr registry to get names of outputs

            // Make the UsdUVTexture reader
            HdMaterialNode fileRead{};
            fileRead.path = materialID.AppendChild(_tokens->imagePlaneTexture);
            fileRead.identifier = UsdImagingTokens->UsdUVTexture;
            fileRead.parameters[HdMayaAdapterTokens->file] = VtValue(
                SdfAssetPath(texturePath.GetText(), texturePath.GetText()));
            network.nodes.push_back(fileRead);

            HdMaterialRelationship rel;
            rel.inputId = fileRead.path;
            rel.inputName = HdMayaAdapterTokens->rgba;
            rel.outputId = materialID;
            rel.outputName = _tokens->colorOpacity;
            network.relationships.push_back(rel);

            // Make the st/uv reader
            HdMaterialNode stRead{};
            stRead.path = materialID.AppendChild(_tokens->imagePlaneStReader);
            stRead.identifier = UsdImagingTokens->UsdPrimvarReader_float2;
            stRead.parameters[_tokens->varname] = VtValue(HdMayaAdapterTokens->st);
            network.nodes.push_back(stRead);

            rel.inputId = stRead.path;
            rel.inputName = HdMayaAdapterTokens->result;
            rel.outputId = fileRead.path;
            rel.outputName = HdMayaAdapterTokens->st;
            network.relationships.push_back(rel);
        }

        network.nodes.push_back(imagePlaneMat);
        map.terminals.push_back(imagePlaneMat.path);
        map.map.emplace(HdMaterialTerminalTokens->surface, network);
        return VtValue(map);
    }

#endif // USD_VERSION_NUM <= 1911

private:
    static void _DirtyMaterialParams(
        MObject& node, MPlug& plug, void* clientData) {
        auto* adapter =
            reinterpret_cast<HdMayaImagePlaneMaterialAdapter*>(clientData);
        if (plug == MayaAttrs::imagePlane::imageName ||
            plug == MayaAttrs::imagePlane::frameExtension ||
            plug == MayaAttrs::imagePlane::frameOffset ||
            plug == MayaAttrs::imagePlane::useFrameExtension) {
            adapter->MarkDirty(HdMaterial::AllDirty);
        }
    }

    inline HdTextureResource::ID _GetTextureResourceID(
        const TfToken& filePath) {
        size_t hash = filePath.Hash();
        boost::hash_combine(
            hash, GetDelegate()->GetParams().textureMemoryPerTexture);
        return HdTextureResource::ID(hash);
    }

    inline TfToken _GetTextureFilePath(
        const MFnDependencyNode& imagePlaneNode) {
        MString imageNameExtracted =
            MRenderUtil::exactImagePlaneFileName(imagePlaneNode.object());
        return TfToken(std::string(imageNameExtracted.asChar()));
    }

    inline HdTextureResourceSharedPtr _GetTextureResource(
        const TfToken& filePath) {
        if (filePath.IsEmpty() || !TfPathExists(filePath)) { return {}; }
        // TODO: handle origin
        auto texture =
            GlfTextureRegistry::GetInstance().GetTextureHandle(filePath);
        // We can't really mimic texture wrapping and mirroring settings from
        // the uv placement node, so we don't touch those for now.
        return HdTextureResourceSharedPtr(new HdStSimpleTextureResource(
            texture, HdTextureType::Uv, HdWrapClamp, HdWrapClamp,
#if USD_VERSION_NUM >= 1910
            HdWrapClamp,
#endif
            HdMinFilterLinearMipmapLinear, HdMagFilterLinear,
            GetDelegate()->GetParams().textureMemoryPerTexture));
    }

#if USD_VERSION_NUM <= 1911
    HdTextureResourceSharedPtr GetTextureResource(
        const TfToken& paramName) override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg(
                "Called "
                "HdMayaImagePlaneMaterialAdapter::GetTextureResource(%s)\n",
                paramName.GetText());
        if (_node == MObject::kNullObj) { return {}; }
        return _GetTextureResource(
            _GetTextureFilePath(MFnDependencyNode(_node)));
    }
#else // USD_VERSION_NUM > 1911
    HdTextureResourceSharedPtr GetTextureResource(
        const SdfPath& textureShaderId) override {
        TF_DEBUG(HDMAYA_ADAPTER_IMAGEPLANES)
            .Msg(
                "Called "
                "HdMayaImagePlaneMaterialAdapter::GetTextureResource(%s)\n",
                textureShaderId.GetText());
        if (_node == MObject::kNullObj) { return {}; }
        return _GetTextureResource(
            _GetTextureFilePath(MFnDependencyNode(_node)));
    }
#endif // USD_VERSION_NUM <= 1911


#if USD_VERSION_NUM <= 1911
    HdTextureResource::ID GetTextureResourceID(
        const TfToken& paramName) override {
        if (_node == MObject::kNullObj) { return {}; }
        return _GetTextureResourceID(
            _GetTextureFilePath(MFnDependencyNode(_node)));
    }
#endif // USD_VERSION_NUM <= 1911


    // So they live long enough
    std::unordered_map<
        TfToken, HdTextureResourceSharedPtr, TfToken::HashFunctor>
        _textureResources;
};

TF_REGISTRY_FUNCTION(TfType) {
    TfType::Define<
        HdMayaImagePlaneMaterialAdapter,
        TfType::Bases<HdMayaMaterialAdapter> >();
}

TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, shadingEngine) {
    HdMayaAdapterRegistry::RegisterMaterialAdapter(
        TfToken("imagePlane"),
        [](const SdfPath& id, HdMayaDelegateCtx* delegate,
           const MObject& obj) -> HdMayaMaterialAdapterPtr {
            return HdMayaMaterialAdapterPtr(
                new HdMayaImagePlaneMaterialAdapter(id, delegate, obj));
        });
}

PXR_NAMESPACE_CLOSE_SCOPE
