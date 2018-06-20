#include <pxr/pxr.h>

#include <pxr/imaging/hd/light.h>
#include <pxr/imaging/glf/simpleLight.h>

#include <maya/MColor.h>
#include <maya/MFnLight.h>
#include <maya/MPlug.h>
#include <maya/MPoint.h>

#include "lightAdapter.h"
#include "adapterRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdMayaPointLightAdapter : public HdMayaLightAdapter {
public:
    HdMayaPointLightAdapter(HdMayaDelegateCtx* delegate, const MDagPath& dag)
        : HdMayaLightAdapter(delegate, dag) {

    }

    void Populate() override {
        GetDelegate()->InsertSprim(HdPrimTypeTokens->simpleLight, GetID(), HdLight::AllDirty);
    }
};


TF_REGISTRY_FUNCTION_WITH_TAG(HdMayaAdapterRegistry, pointLight) {
    HdMayaAdapterRegistry::RegisterDagAdapter("pointLight",
    [](HdMayaDelegateCtx* delegate, const MDagPath& dag) -> std::shared_ptr<HdMayaDagAdapter> {
        return std::static_pointer_cast<HdMayaDagAdapter>(std::make_shared<HdMayaPointLightAdapter>(delegate, dag));
    });
}

PXR_NAMESPACE_CLOSE_SCOPE
