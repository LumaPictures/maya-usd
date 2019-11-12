//
// Copyright 2017 Animal Logic
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

#include "AL/maya/utils/MayaHelperMacros.h"
#include "AL/usdmaya/TypeIDs.h"
#include "AL/usdmaya/DebugCodes.h"
#include "AL/usdmaya/nodes/ProxyShape.h"
#include "AL/usdmaya/nodes/Transform.h"
#include "AL/usdmaya/nodes/TransformationMatrix.h"
#include "AL/usdmaya/utils/AttributeType.h"
#include "AL/usdmaya/utils/Utils.h"

#include "maya/MFileIO.h"
#include "maya/MGlobal.h"
#include "maya/MViewport2Renderer.h"
#include "maya/MFnTransform.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace AL {
namespace usdmaya {
namespace nodes {

using AL::usdmaya::utils::UsdDataType;

//----------------------------------------------------------------------------------------------------------------------
const MTypeId TransformationMatrix::kTypeId(AL_USDMAYA_TRANSFORMATION_MATRIX);

//----------------------------------------------------------------------------------------------------------------------
MPxTransformationMatrix* TransformationMatrix::creator()
{
  return new TransformationMatrix;
}

//----------------------------------------------------------------------------------------------------------------------
TransformationMatrix::TransformationMatrix()
  : MPxTransformationMatrix(),
    m_prim(),
    m_xform(),
    m_time(UsdTimeCode::Default()),
    m_scaleTweak(0, 0, 0),
    m_rotationTweak(0, 0, 0),
    m_translationTweak(0, 0, 0),
    m_shearTweak(0, 0, 0),
    m_scalePivotTweak(0, 0, 0),
    m_scalePivotTranslationTweak(0, 0, 0),
    m_rotatePivotTweak(0, 0, 0),
    m_rotatePivotTranslationTweak(0, 0, 0),
    m_rotateOrientationTweak(0, 0, 0, 1.0),
    m_scaleFromUsd(1.1, 1.1, 1.1),
    m_rotationFromUsd(0, 0, 0),
    m_translationFromUsd(0.1, 0.2, 0.3),
    m_shearFromUsd(0, 0, 0),
    m_scalePivotFromUsd(0, 0, 0),
    m_scalePivotTranslationFromUsd(0, 0, 0),
    m_rotatePivotFromUsd(0, 0, 0),
    m_rotatePivotTranslationFromUsd(0, 0, 0),
    m_rotateOrientationFromUsd(0, 0, 0, 1.0),
    m_localTranslateOffset(0, 0, 0),
    m_flags(0)
{
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::TransformationMatrix\n");
  initialiseToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
TransformationMatrix::TransformationMatrix(const UsdPrim& prim)
  : MPxTransformationMatrix(),
    m_prim(prim),
    m_xform(prim),
    m_time(UsdTimeCode::Default()),
    m_scaleTweak(0, 0, 0),
    m_rotationTweak(0, 0, 0),
    m_translationTweak(0, 0, 0),
    m_shearTweak(0, 0, 0),
    m_scalePivotTweak(0, 0, 0),
    m_scalePivotTranslationTweak(0, 0, 0),
    m_rotatePivotTweak(0, 0, 0),
    m_rotatePivotTranslationTweak(0, 0, 0),
    m_rotateOrientationTweak(0, 0, 0, 1.0),
    m_scaleFromUsd(1.0, 1.0, 1.0),
    m_rotationFromUsd(0, 0, 0),
    m_translationFromUsd(0, 0, 0),
    m_shearFromUsd(0, 0, 0),
    m_scalePivotFromUsd(0, 0, 0),
    m_scalePivotTranslationFromUsd(0, 0, 0),
    m_rotatePivotFromUsd(0, 0, 0),
    m_rotatePivotTranslationFromUsd(0, 0, 0),
    m_rotateOrientationFromUsd(0, 0, 0, 1.0),
    m_localTranslateOffset(0, 0, 0),
    m_flags(0)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::TransformationMatrix\n");
  initialiseToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::setPrimInternal(const UsdPrim& prim, Transform* transformNode)
{
  if(prim.IsValid())
  {
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setPrimInternal %s\n", prim.GetName().GetText());
    m_prim = prim;
    UsdGeomXform xform(prim);
    m_xform = xform;
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setPrimInternal null\n");
    m_prim = UsdPrim();
    m_xform = UsdGeomXform();
  }
  // Most of these flags are calculated based on reading the usd prim; however, a few are driven
  // "externally" (ie, from attributes on the controlling transform node), and should NOT be reset
  // when we're re-initializing
  m_flags &= kPreservationMask;
  m_scaleTweak = MVector(0, 0, 0);
  m_rotationTweak = MEulerRotation(0, 0, 0);
  m_translationTweak = MVector(0, 0, 0);
  m_shearTweak = MVector(0, 0, 0);
  m_scalePivotTweak = MPoint(0, 0, 0);
  m_scalePivotTranslationTweak = MVector(0, 0, 0);
  m_rotatePivotTweak = MPoint(0, 0, 0);
  m_rotatePivotTranslationTweak = MVector(0, 0, 0);
  m_rotateOrientationTweak = MQuaternion(0, 0, 0, 1.0);
  m_localTranslateOffset = MVector(0, 0, 0);

  if(m_prim.IsValid())
  {
    m_scaleFromUsd = MVector(1.0, 1.0, 1.0);
    m_rotationFromUsd = MEulerRotation(0, 0, 0);
    m_translationFromUsd = MVector(0, 0, 0);
    m_shearFromUsd = MVector(0, 0, 0);
    m_scalePivotFromUsd = MPoint(0, 0, 0);
    m_scalePivotTranslationFromUsd = MVector(0, 0, 0);
    m_rotatePivotFromUsd = MPoint(0, 0, 0);
    m_rotatePivotTranslationFromUsd = MVector(0, 0, 0);
    m_rotateOrientationFromUsd = MQuaternion(0, 0, 0, 1.0);
    initialiseToPrim(!MFileIO::isReadingFile(), transformNode);
    MPxTransformationMatrix::scaleValue = m_scaleFromUsd;
    MPxTransformationMatrix::rotationValue = m_rotationFromUsd;
    MPxTransformationMatrix::translationValue = m_translationFromUsd;
    MPxTransformationMatrix::shearValue = m_shearFromUsd;
    MPxTransformationMatrix::scalePivotValue = m_scalePivotFromUsd;
    MPxTransformationMatrix::scalePivotTranslationValue = m_scalePivotTranslationFromUsd;
    MPxTransformationMatrix::rotatePivotValue = m_rotatePivotFromUsd;
    MPxTransformationMatrix::rotatePivotTranslationValue = m_rotatePivotTranslationFromUsd;
    MPxTransformationMatrix::rotateOrientationValue = m_rotateOrientationFromUsd;
  }
}

//----------------------------------------------------------------------------------------------------------------------
const UsdMayaXformStack&
TransformationMatrix::MayaSinglePivotStack()
{
    static UsdMayaXformStack mayaSinglePivotStack(
            // ops
            {
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->translate,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotatePivotTranslate,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->pivot,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotate,
                        UsdGeomXformOp::TypeRotateXYZ),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->rotateAxis,
                        UsdGeomXformOp::TypeRotateXYZ),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->scalePivotTranslate,
                        UsdGeomXformOp::TypeTranslate),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->shear,
                        UsdGeomXformOp::TypeTransform),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->scale,
                        UsdGeomXformOp::TypeScale),
                UsdMayaXformOpClassification(
                        UsdMayaXformStackTokens->pivot,
                        UsdGeomXformOp::TypeTranslate,
                        true /* isInvertedTwin */)
            },

            // inversionTwins
            {
                {2, 8},
            });

    return mayaSinglePivotStack;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readVector(MVector& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readVector\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kVec3d:
    {
      GfVec3d value;
      const bool retValue = op.GetAs<GfVec3d>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = value[0];
      result.y = value[1];
      result.z = value[2];
    }
    break;

  case UsdDataType::kVec3f:
    {
      GfVec3f value;
      const bool retValue = op.GetAs<GfVec3f>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  case UsdDataType::kVec3h:
    {
      GfVec3h value;
      const bool retValue = op.GetAs<GfVec3h>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  case UsdDataType::kVec3i:
    {
      GfVec3i value;
      const bool retValue = op.GetAs<GfVec3i>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  default:
    return false;
  }

  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readVector %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushVector(const MVector& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushVector %f %f %f [@%f]\n%s\n", result.x, result.y, result.z, timeCode.GetValue(), op.GetOpName().GetText());
  auto attr = op.GetAttr();
  if(!attr)
  {
    return false;
  }

  TfToken typeName;
  attr.GetMetadata(SdfFieldKeys->TypeName, &typeName);
  SdfValueTypeName vtn = SdfSchema::GetInstance().FindType(typeName);
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kVec3d:
    {
      GfVec3d value(result.x, result.y, result.z);
      GfVec3d oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
      {
        op.Set(value, timeCode);
      }
    }
    break;

  case UsdDataType::kVec3f:
    {
      GfVec3f value(result.x, result.y, result.z);
      GfVec3f oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
      {
        op.Set(value, timeCode);
      }
    }
    break;

  case UsdDataType::kVec3h:
    {
      GfVec3h value(result.x, result.y, result.z);
      GfVec3h oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
      {
        op.Set(value, timeCode);
      }
    }
    break;

  case UsdDataType::kVec3i:
    {
      GfVec3i value(result.x, result.y, result.z);
      GfVec3i oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
      {
        op.Set(value, timeCode);
      }
    }
    break;

  default:
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushShear(const MVector& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushShear %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kMatrix4d:
    {
      GfMatrix4d m(
          1.0,      0.0,      0.0, 0.0,
          result.x, 1.0,      0.0, 0.0,
          result.y, result.z, 1.0, 0.0,
          0.0,      0.0,      0.0, 1.0);
      GfMatrix4d oldValue;
      op.Get(&oldValue, timeCode);
      if(m != oldValue)
        op.Set(m, timeCode);
    }
    break;

  default:
    return false;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readShear(MVector& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readShear\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kMatrix4d:
    {
      GfMatrix4d value;
      const bool retValue = op.GetAs<GfMatrix4d>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = value[1][0];
      result.y = value[2][0];
      result.z = value[2][1];
    }
    break;

  default:
    return false;
  }
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readShear %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readPoint(MPoint& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readPoint\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kVec3d:
    {
      GfVec3d value;
      const bool retValue = op.GetAs<GfVec3d>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = value[0];
      result.y = value[1];
      result.z = value[2];
    }
    break;

  case UsdDataType::kVec3f:
    {
      GfVec3f value;
      const bool retValue = op.GetAs<GfVec3f>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  case UsdDataType::kVec3h:
    {
      GfVec3h value;
      const bool retValue = op.GetAs<GfVec3h>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  case UsdDataType::kVec3i:
    {
      GfVec3i value;
      const bool retValue = op.GetAs<GfVec3i>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      result.x = double(value[0]);
      result.y = double(value[1]);
      result.z = double(value[2]);
    }
    break;

  default:
    return false;
  }
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readPoint %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readMatrix(MMatrix& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readMatrix\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kMatrix4d:
    {
      GfMatrix4d value;
      const bool retValue = op.GetAs<GfMatrix4d>(&value, timeCode);
      if (!retValue)
      {
        return false;
      }
      auto vtemp = (const void*)&value;
      auto mtemp = (const MMatrix*)vtemp;
      result = *mtemp;
    }
    break;

  default:
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushMatrix(const MMatrix& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushMatrix\n");
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kMatrix4d:
    {
      const GfMatrix4d& value = *(const GfMatrix4d*)(&result);
      GfMatrix4d oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
      {
        const bool retValue = op.Set<GfMatrix4d>(value, timeCode);
        if (!retValue)
        {
          return false;
        }
      }
    }
    break;

  default:
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::setFromMatrix(const MMatrix& m)
{
  decomposeMatrix(m);
  m_scaleFromUsd = scaleValue;
  m_rotationFromUsd = rotationValue;
  m_translationFromUsd = translationValue;
  m_shearFromUsd = shearValue;
  m_scalePivotFromUsd = scalePivotValue;
  m_scalePivotTranslationFromUsd = scalePivotTranslationValue;
  m_rotatePivotFromUsd = rotatePivotValue;
  m_rotatePivotTranslationFromUsd = rotatePivotTranslationValue;
  m_rotateOrientationFromUsd = rotateOrientationValue;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::setFromPrimMatrix()
{
  if (!m_xform)
  {
    TF_WARN("Cannot set TransformationMatrix from usd prim if m_xform not set");
    return;
  }

  GfMatrix4d matrix;
  bool resetsXformStack;

  // Note that if GetLocalTransformation returns false, it may just be because
  // underlying prim has no xformOps defined at all...
  if(m_xform.GetLocalTransformation(&matrix, &resetsXformStack, getTimeCode()))
  {
    setFromMatrix(AL::usdmaya::utils::matrixToMMatrix(matrix));
    if(resetsXformStack)
    {
      m_flags &= ~kInheritsTransform;
    }
    else
    {
      m_flags |= kInheritsTransform;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushPoint(const MPoint& result, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushPoint %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  const SdfValueTypeName vtn = op.GetTypeName();
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(vtn);
  switch(attr_type)
  {
  case UsdDataType::kVec3d:
    {
      GfVec3d value(result.x, result.y, result.z);
      GfVec3d oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
        op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3f:
    {
      GfVec3f value(result.x, result.y, result.z);
      GfVec3f oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
        op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3h:
    {
      GfVec3h value(result.x, result.y, result.z);
      GfVec3h oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
        op.Set(value, timeCode);
    }
    break;

  case UsdDataType::kVec3i:
    {
      GfVec3i value(result.x, result.y, result.z);
      GfVec3i oldValue;
      op.Get(&oldValue, timeCode);
      if(value != oldValue)
        op.Set(value, timeCode);
    }
    break;

  default:
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------------------------------------------------
double TransformationMatrix::readDouble(const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readDouble\n");
  double result = 0;
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(op.GetTypeName());
  switch(attr_type)
  {
  case UsdDataType::kHalf:
    {
      GfHalf value;
      const bool retValue = op.Get<GfHalf>(&value, timeCode);
      if (retValue)
      {
        result = float(value);
      }
    }
    break;

  case UsdDataType::kFloat:
    {
      float value;
      const bool retValue = op.Get<float>(&value, timeCode);
      if (retValue)
      {
        result = double(value);
      }
    }
    break;

  case UsdDataType::kDouble:
    {
      double value;
      const bool retValue = op.Get<double>(&value, timeCode);
      if (retValue)
      {
        result = value;
      }
    }
    break;

  case UsdDataType::kInt:
    {
      int32_t value;
      const bool retValue = op.Get<int32_t>(&value, timeCode);
      if (retValue)
      {
        result = double(value);
      }
    }
    break;

  default:
    break;
  }
  TF_DEBUG(ALUSDMAYA_EVALUATION).Msg("TransformationMatrix::readDouble %f\n%s\n", result, op.GetOpName().GetText());
  return result;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushDouble(const double value, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushDouble %f\n%s\n", value, op.GetOpName().GetText());
  UsdDataType attr_type = AL::usdmaya::utils::getAttributeType(op.GetTypeName());
  switch(attr_type)
  {
  case UsdDataType::kHalf:
    {
      GfHalf oldValue;
      op.Get(&oldValue);
      if(oldValue != GfHalf(value))
        op.Set(GfHalf(value), timeCode);
    }
    break;

  case UsdDataType::kFloat:
    {
      float oldValue;
      op.Get(&oldValue);
      if(oldValue != float(value))
        op.Set(float(value), timeCode);
    }
    break;

  case UsdDataType::kDouble:
    {
      double oldValue;
      op.Get(&oldValue);
      if(oldValue != double(value))
        op.Set(double(value), timeCode);
    }
    break;

  case UsdDataType::kInt:
    {
      int32_t oldValue;
      op.Get(&oldValue);
      if(oldValue != int32_t(value))
        op.Set(int32_t(value), timeCode);
    }
    break;

  default:
    break;
  }
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::readRotation(MEulerRotation& result, const UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::readRotation %f %f %f\n%s\n", result.x, result.y, result.z, op.GetOpName().GetText());
  const double degToRad = 3.141592654 / 180.0;
  switch(op.GetOpType())
  {
  case UsdGeomXformOp::TypeRotateX:
    {
      result.x = readDouble(op, timeCode) * degToRad;
      result.y = 0.0;
      result.z = 0.0;
      result.order = MEulerRotation::kXYZ;
    }
    break;

  case UsdGeomXformOp::TypeRotateY:
    {
      result.x = 0.0;
      result.y = readDouble(op, timeCode) * degToRad;
      result.z = 0.0;
      result.order = MEulerRotation::kXYZ;
    }
    break;

  case UsdGeomXformOp::TypeRotateZ:
    {
      result.x = 0.0;
      result.y = 0.0;
      result.z = readDouble(op, timeCode) * degToRad;
      result.order = MEulerRotation::kXYZ;
    }
    break;

  case UsdGeomXformOp::TypeRotateXYZ:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kXYZ;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateXZY:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kXZY;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateYXZ:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kYXZ;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateYZX:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kYZX;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateZXY:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kZXY;
      }
      else
        return false;
    }
    break;

  case UsdGeomXformOp::TypeRotateZYX:
    {
      MVector v;
      if(readVector(v, op, timeCode))
      {
        result.x = v.x * degToRad;
        result.y = v.y * degToRad;
        result.z = v.z * degToRad;
        result.order = MEulerRotation::kZYX;
      }
      else
        return false;
    }
    break;

  default:
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
bool TransformationMatrix::pushRotation(const MEulerRotation& value, UsdGeomXformOp& op, UsdTimeCode timeCode)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotation %f %f %f\n%s\n", value.x, value.y, value.z, op.GetOpName().GetText());
  const double radToDeg = 180.0 / 3.141592654;
  switch(op.GetOpType())
  {
  case UsdGeomXformOp::TypeRotateX:
    {
      pushDouble(value.x * radToDeg, op, timeCode);
    }
    break;

  case UsdGeomXformOp::TypeRotateY:
    {
      pushDouble(value.y * radToDeg, op, timeCode);
    }
    break;

  case UsdGeomXformOp::TypeRotateZ:
    {
      pushDouble(value.z * radToDeg, op, timeCode);
    }
    break;

  case UsdGeomXformOp::TypeRotateXYZ:
  case UsdGeomXformOp::TypeRotateXZY:
  case UsdGeomXformOp::TypeRotateYXZ:
  case UsdGeomXformOp::TypeRotateYZX:
  case UsdGeomXformOp::TypeRotateZYX:
  case UsdGeomXformOp::TypeRotateZXY:
    {
      MVector v(value.x, value.y, value.z);
      v *= radToDeg;
      return pushVector(v, op, timeCode);
    }
    break;

  default:
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::initialiseToPrim(bool readFromPrim, Transform* transformNode)
{
  // if not yet initialized, do not execute this code! (It will crash!).
  if(!m_prim || !m_xform)
    return;

  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::initialiseToPrim: %s\n",
      m_prim.GetPath().GetText());

  bool resetsXformStack = false;
  m_xformops = m_xform.GetOrderedXformOps(&resetsXformStack);
  m_orderedOps.clear();
  m_orderedOpMayaIndices.clear();

  if(!resetsXformStack)
    m_flags |= kInheritsTransform;

  if (m_xformops.empty())
  {
    // An empty xform matches anything, so we'll say it matches maya...
    m_flags |= kFromMayaSchema;
  }
  else
  {
    static const std::pair<const UsdMayaXformStack&, uint32_t> stackFlagPairs[3] = {
        {UsdMayaXformStack::MayaStack(), kFromMayaSchema},
        {MayaSinglePivotStack(), kSinglePivotSchema},
        {UsdMayaXformStack::MatrixStack(), kFromMatrix},
    };
    for (const auto& stackFlagPair : stackFlagPairs)
    {
      const auto& stack = stackFlagPair.first;
      const auto flag = stackFlagPair.second;
      m_orderedOps = stack.MatchingSubstack(m_xformops);
      if (!m_orderedOps.empty())
      {
        m_flags |= flag;
        break;
      }
    }
  }

  if(m_flags & kAnyKnownSchema)
  {
    auto opIt = m_orderedOps.begin();
    for(std::vector<UsdGeomXformOp>::const_iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
    {
      const UsdMayaXformOpClassification& opClass = *opIt;
      if (opClass.IsInvertedTwin()) continue;

      const UsdGeomXformOp& op = *it;
      const TfToken& opName = opClass.GetName();
      if (opName == UsdMayaXformStackTokens->translate)
      {
        m_flags |= kPrimHasTranslation;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedTranslation;
        }
        if(readFromPrim)
        {
          MVector tempTranslation;
          internal_readVector(tempTranslation, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::translateX).setValue(tempTranslation.x);
            MPlug(transformNode->thisMObject(), MPxTransform::translateY).setValue(tempTranslation.y);
            MPlug(transformNode->thisMObject(), MPxTransform::translateZ).setValue(tempTranslation.z);
          }
          m_translationFromUsd = tempTranslation;
        }
      }
      else if (opName == UsdMayaXformStackTokens->pivot)
      {
        m_flags |= kPrimHasPivot;
        if(readFromPrim)
        {
          internal_readPoint(m_scalePivotFromUsd, op);
          m_rotatePivotFromUsd = m_scalePivotFromUsd;
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotX).setValue(m_rotatePivotFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotY).setValue(m_rotatePivotFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotZ).setValue(m_rotatePivotFromUsd.z);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotX).setValue(m_scalePivotFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotY).setValue(m_scalePivotFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotZ).setValue(m_scalePivotFromUsd.z);
          }
        }
      }
      else if (opName == UsdMayaXformStackTokens->rotatePivotTranslate)
      {
        m_flags |= kPrimHasRotatePivotTranslate;
        if(readFromPrim)
        {
          internal_readVector(m_rotatePivotTranslationFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateX).setValue(m_rotatePivotTranslationFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateY).setValue(m_rotatePivotTranslationFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotTranslateZ).setValue(m_rotatePivotTranslationFromUsd.z);
          }
        }
      }
      else if (opName == UsdMayaXformStackTokens->rotatePivot)
      {
        m_flags |= kPrimHasRotatePivot;
        if(readFromPrim)
        {
          internal_readPoint(m_rotatePivotFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotX).setValue(m_rotatePivotFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotY).setValue(m_rotatePivotFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotatePivotZ).setValue(m_rotatePivotFromUsd.z);
          }
        }
      }
      else if (opName == UsdMayaXformStackTokens->rotate)
      {
        m_flags |= kPrimHasRotation;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedRotation;
        }
        if(readFromPrim)
        {
          internal_readRotation(m_rotationFromUsd, op);
          if(transformNode)
          {
            m_rotationTweak[0] = m_rotationTweak[1] = m_rotationTweak[2] = 0;
            // attempting to set the rotation via the attributes can end up failing when using zxy rotation orders. 
            // The only reliable way to set this value would appeear to be via MFnTransform :(
            MFnTransform fn(m_transformNode.object());
            fn.setRotation(m_rotationFromUsd);
          }
        }
      }
      else if (opName == UsdMayaXformStackTokens->rotateAxis)
      {
        m_flags |= kPrimHasRotateAxes;
        if(readFromPrim) {
          MEulerRotation eulers;
          internal_readRotation(eulers, op);
          m_rotateOrientationFromUsd = eulers.asQuaternion();
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisX).setValue(eulers.x);
            MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisY).setValue(eulers.y);
            MPlug(transformNode->thisMObject(), MPxTransform::rotateAxisZ).setValue(eulers.z);
          }
        }
      }
      else if (opName == UsdMayaXformStackTokens->scalePivotTranslate)
      {
        m_flags |= kPrimHasScalePivotTranslate;
        if(readFromPrim)
        {
          internal_readVector(m_scalePivotTranslationFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateX).setValue(m_scalePivotTranslationFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateY).setValue(m_scalePivotTranslationFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotTranslateZ).setValue(m_scalePivotTranslationFromUsd.z);
          }
        }
      }
      else if (opName == UsdMayaXformStackTokens->scalePivot)
      {
        m_flags |= kPrimHasScalePivot;
        if(readFromPrim)
        {
          internal_readPoint(m_scalePivotFromUsd, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotX).setValue(m_scalePivotFromUsd.x);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotY).setValue(m_scalePivotFromUsd.y);
            MPlug(transformNode->thisMObject(), MPxTransform::scalePivotZ).setValue(m_scalePivotFromUsd.z);
          }
        }
      }
      else if (opName == UsdMayaXformStackTokens->shear)
      {
        m_flags |= kPrimHasShear;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedShear;
        }
        if(readFromPrim)
        {
          MVector tempShear;
          internal_readShear(tempShear, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::shearXY).setValue(tempShear.x);
            MPlug(transformNode->thisMObject(), MPxTransform::shearXZ).setValue(tempShear.y);
            MPlug(transformNode->thisMObject(), MPxTransform::shearYZ).setValue(tempShear.z);
          }
          m_shearFromUsd = tempShear;
        }
      }
      else if (opName == UsdMayaXformStackTokens->scale)
      {
        m_flags |= kPrimHasScale;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedScale;
        }
        if(readFromPrim)
        {
          MVector tempScale(1.0,1.0,1.0);
          internal_readVector(tempScale, op);
          if(transformNode)
          {
            MPlug(transformNode->thisMObject(), MPxTransform::scaleX).setValue(tempScale.x);
            MPlug(transformNode->thisMObject(), MPxTransform::scaleY).setValue(tempScale.y);
            MPlug(transformNode->thisMObject(), MPxTransform::scaleZ).setValue(tempScale.z);
          }
          m_scaleFromUsd = tempScale;
        }
        
      }
      else if (opName == UsdMayaXformStackTokens->transform)
      {
        m_flags |= kPrimHasTransform;
        m_flags |= kFromMatrix;
        m_flags |= kPushPrimToMatrix;
        if(op.GetNumTimeSamples() > 1)
        {
          m_flags |= kAnimatedMatrix;
        }

        if(readFromPrim)
        {
          MMatrix m;
          internal_readMatrix(m, m_xformops[0]);
          setFromMatrix(m);
        }
      }
      else
      {
        std::cerr << "TransformationMatrix::initialiseToPrim - Invalid transform operation: " << opName.GetText() << std::endl;
      }

    }
    assert(m_orderedOps.size() == m_xformops.size());
  }
  else
  {
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::initialiseToPrim - prim xform ops did not match any known"
        " xformStack: %s\n", m_prim.GetPath().GetText());
    setFromPrimMatrix();
  }

  // if some animation keys are found on the transform ops, assume we have a read only viewer of the transform data.
  if(m_flags & kAnimationMask)
  {
    m_flags &= ~kPushToPrimEnabled;
    m_flags |= kReadAnimatedValues;
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::updateToTime(const UsdTimeCode& time)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::updateToTime %f\n", time.GetValue());
  // if not yet initialized, do not execute this code! (It will crash!).
  if(!m_prim)
  {
    return;
  }
  if(m_time != time)
  {
    m_time = time;
    if (m_flags & kAnyKnownSchema)
    {
      assert(m_orderedOps.size() == m_xformops.size());
      auto opIt = m_orderedOps.begin();
      for(std::vector<UsdGeomXformOp>::const_iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
      {
        const UsdGeomXformOp& op = *it;
        const TfToken& opName = opIt->GetName();
        if (opName == UsdMayaXformStackTokens->translate)
        {
          if(op.GetNumTimeSamples() >= 1)
          {
            m_flags |= kAnimatedTranslation;
            internal_readVector(m_translationFromUsd, op);
            MPxTransformationMatrix::translationValue = m_translationFromUsd + m_translationTweak;
          }
        }
        else if (opName == UsdMayaXformStackTokens->rotate)
        {
          if(op.GetNumTimeSamples() >= 1)
          {
            m_flags |= kAnimatedRotation;
            internal_readRotation(m_rotationFromUsd, op);
            MPxTransformationMatrix::rotationValue = m_rotationFromUsd;
            MPxTransformationMatrix::rotationValue.x += m_rotationTweak.x;
            MPxTransformationMatrix::rotationValue.y += m_rotationTweak.y;
            MPxTransformationMatrix::rotationValue.z += m_rotationTweak.z;
          }
        }
        else if (opName == UsdMayaXformStackTokens->scale)
        {
          if(op.GetNumTimeSamples() >= 1)
          {
            m_flags |= kAnimatedScale;
            internal_readVector(m_scaleFromUsd, op);
            MPxTransformationMatrix::scaleValue = m_scaleFromUsd + m_scaleTweak;
          }
        }
        else if (opName == UsdMayaXformStackTokens->shear)
        {
          if(op.GetNumTimeSamples() >= 1)
          {
            m_flags |= kAnimatedShear;
            internal_readShear(m_shearFromUsd, op);
            MPxTransformationMatrix::shearValue = m_shearFromUsd + m_shearTweak;
          }
        }
        else if (opName == UsdMayaXformStackTokens->transform)
        {
          if(op.GetNumTimeSamples() >= 1)
          {
            m_flags |= kAnimatedMatrix;
            GfMatrix4d matrix;
            op.Get<GfMatrix4d>(&matrix, getTimeCode());
            // We can't use MPxTransformationMatrix::decomposeMatrix directly, as we need to add in tweak values
            MTransformationMatrix mayaXform = AL::usdmaya::utils::matrixToMTransformationMatrix(matrix);
            m_rotationFromUsd = mayaXform.eulerRotation();
            m_translationFromUsd = mayaXform.getTranslation(MSpace::kObject);
            double tempDoubles[3];
            mayaXform.getScale(tempDoubles, MSpace::kObject);
            m_scaleFromUsd.x = tempDoubles[0];
            m_scaleFromUsd.y = tempDoubles[1];
            m_scaleFromUsd.z = tempDoubles[2];
            mayaXform.getShear(tempDoubles, MSpace::kObject);
            m_shearFromUsd.x = tempDoubles[0];
            m_shearFromUsd.y = tempDoubles[1];
            m_shearFromUsd.z = tempDoubles[2];
            MPxTransformationMatrix::rotationValue.x = m_rotationFromUsd.x + m_rotationTweak.x;
            MPxTransformationMatrix::rotationValue.y = m_rotationFromUsd.y + m_rotationTweak.y;
            MPxTransformationMatrix::rotationValue.z = m_rotationFromUsd.z + m_rotationTweak.z;
            MPxTransformationMatrix::translationValue = m_translationFromUsd + m_translationTweak;
            MPxTransformationMatrix::scaleValue = m_scaleFromUsd + m_scaleTweak;
            MPxTransformationMatrix::shearValue = m_shearFromUsd + m_shearTweak;
          }
        }
      }
    }
    else
    {
      setFromPrimMatrix();
    }
  }
}

void TransformationMatrix::buildOrderedOpMayaIndices()
{
  if (m_orderedOpMayaIndices.empty() && !m_orderedOps.empty())
  {
    // fill out m_orderedOpMayaIndices, so we know where to insert stuff
    if (m_flags & kFromMayaSchema)
    {
      const auto& mayaStack = UsdMayaXformStack::MayaStack();
      m_orderedOpMayaIndices.reserve(m_orderedOps.size());
      for(auto& op : m_orderedOps)
      {
        m_orderedOpMayaIndices.push_back(mayaStack.FindOpIndex(op.GetName(), op.IsInvertedTwin()));
      }
    }
    else if (m_flags & kSinglePivotSchema)
    {
      const auto& mayaStack = UsdMayaXformStack::MayaStack();
      m_orderedOpMayaIndices.reserve(m_orderedOps.size());
      for(auto& op : m_orderedOps)
      {
        // The only op in the common stack that has a different name than in the maya stack
        // is the "pivot" op - for that, we consider the non-inverted version to have the same
        // place as non-inverted rotatePivot, and the inverted version to have the same place
        // s the inverted scalePivot, since that will give the same xform if we guarantee that
        // rotatePivot == scalePivot... which we do
        TfToken name = op.GetName();
        bool isInverted = op.IsInvertedTwin();
        if (name == UsdMayaXformStackTokens->pivot)
        {
          if (isInverted)
          {
            name = UsdMayaXformStackTokens->scalePivot;
          }
          else
          {
            name = UsdMayaXformStackTokens->rotatePivot;
          }
        }
        m_orderedOpMayaIndices.push_back(mayaStack.FindOpIndex(name, isInverted));
      }
    }
  }
}

MStatus TransformationMatrix::insertAndPushPivotOp(const TfToken& pivotName)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertAndPushPivotOp - %s\n", pivotName.GetText());

  bool insertScalePivot = false;
  bool insertRotatePivot = false;

  if (pivotName == UsdMayaXformStackTokens->scalePivot)
  {
    insertScalePivot = true;
  }
  else if (pivotName == UsdMayaXformStackTokens->rotatePivot)
  {
    insertRotatePivot = true;
  }
  else
  {
    TF_CODING_ERROR("Unknown pivot name: %s", pivotName.GetText());
    return MStatus::kFailure;
  }

  // We only need to split if we have a singular pivot
  if (primHasPivot())
  {
    if (MPxTransformationMatrix::scalePivotValue == MPxTransformationMatrix::rotatePivotValue)
    {
      // If they are equal, then we push the generic pivot, not the split ones
      TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertAndPushPivotOp - pushing generic pivot\n");
      pushPivotToPrim();
      // If we still have a generic pivot, we shouldn't have / need to do anything
      // with split pivots - return immediately
      return MStatus::kSuccess;
    }

    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertAndPushPivotOp - splitting pivot\n");

    // We will need to split out the pivot... we do this by first REMOVING
    // our singular pivot op...
    AL_MAYA_CHECK_ERROR(
        removeOp(UsdMayaXformStackTokens->pivot, kPrimHasPivot),
        "Error removing singular pivot op");

    // ...then we just make sure we insert both types of pivot ops.
    insertRotatePivot = true;
    insertScalePivot = true;
  }

  if (insertScalePivot)
  {
    AL_MAYA_CHECK_ERROR(insertScalePivotOp(), "Error inserting scalePivot op");
    // Make sure we push to set initial value
    pushScalePivotToPrim();
  }
  if (insertRotatePivot)
  {
    AL_MAYA_CHECK_ERROR(insertRotatePivotOp(), "Error inserting rotatePivot op");
    // Make sure we push to set initial value
    pushRotatePivotToPrim();
  }

  return MStatus::kSuccess;
}

MStatus TransformationMatrix::removeOp(
    const TfToken& opName,
    Flags oldFlag)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::removeOp - %s\n", opName.GetText());

  // Build out the m_orderedOpMayaIndices so we know where to remove things - delayed
  // till now, since most xforms won't be altered / have ops removed, and won't need this
  buildOrderedOpMayaIndices();

  // We need to find which op(s) to remove; note that we can't
  // rely on m_orderedOpMayaIndices to speed up where to find our op,
  // because the op we're removing may not be an op from the MayaStack... so
  // we just iterate through m_orderedOps. This should be ok, since m_orderedOps
  // is never that big, and we likely won't be removing ops that often...
  bool foundOne = false;
  // Iterate backwards, so the indices will remain valid even if we remove an item...

  for (size_t i = m_orderedOps.size() - 1; ; --i)
  {
    if (opName == m_orderedOps[i].GetName())
    {
      m_orderedOps.erase(m_orderedOps.begin() + i);
      m_xformops.erase(m_xformops.begin() + i);
      if (!m_orderedOpMayaIndices.empty())
      {
        m_orderedOpMayaIndices.erase(m_orderedOpMayaIndices.begin() + i);
      }
      // If this is the second op we've found, we can abort, since a stack should never
      // have more than two w/ the same name...
      if (foundOne) break;
      foundOne = true;
    }
    // Using unsigned, so need to check BEFORE decrementing... could check for,
    // ie, i == reinterpret_cast<size_t>(-1), but that makes me nervous...
    if (i == 0) break;
  }

  assert(m_orderedOps.size() == m_xformops.size());

  m_flags &= ~oldFlag;
  if (!foundOne)
  {
    return MStatus::kFailure;
  }
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  return MStatus::kSuccess;
}


MStatus TransformationMatrix::insertOp(
    UsdGeomXformOp::Type opType,
    UsdGeomXformOp::Precision precision,
    const TfToken& opName,
    Flags newFlag,
    bool insertAtBeginning)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertOp - %s\n", opName.GetText());

  // Build out the m_orderedOpMayaIndices so we know where to insert things - delayed
  // till now, since most xforms won't be altered / have ops inserted, and won't need this
  buildOrderedOpMayaIndices();

  // If we currently have a singular "pivot" op, and we're trying to insert a rotatePivot or
  // scalePivot, first see if it's actually necessary, and if so, "convert" our op stack to use
  // split rotatePivot and scalePivot


  // Find an iterator pointing to the location in m_orderedOps where the given
  // maya operator should be inserted. Note that opIndex must refer to an entry in MayaStack
  // (not CommonStack, etc)
  auto findOpInsertPos = [&](size_t opIndex) -> int {

    assert(opIndex != UsdMayaXformStack::NO_INDEX);
    assert(opIndex < UsdMayaXformStack::MayaStack().GetOps().size());

    auto indexIter = std::lower_bound(m_orderedOpMayaIndices.begin(),
        m_orderedOpMayaIndices.end(), opIndex);
    return indexIter - m_orderedOpMayaIndices.begin();
  };

  auto addOp = [&](
      size_t opIndex,
      bool insertAtBeginning) -> int {
    assert(opIndex != UsdMayaXformStack::NO_INDEX);

    auto& mayaStack = UsdMayaXformStack::MayaStack();
    const UsdMayaXformOpClassification& opClass = mayaStack[opIndex];
    UsdGeomXformOp op = m_xform.AddXformOp(opType, precision, opName, opClass.IsInvertedTwin());
    if (!op)
    {
      return -1;
    }

    // insert our op into the correct stack location
    auto insertIndex = insertAtBeginning ? 0 : findOpInsertPos(opIndex);
    m_orderedOps.insert(m_orderedOps.begin() + insertIndex, opClass);
    m_xformops.insert(m_xformops.begin() + insertIndex, op);
    m_orderedOpMayaIndices.insert(m_orderedOpMayaIndices.begin() + insertIndex, opIndex);

    assert(m_orderedOps.size() == m_xformops.size());

    return insertIndex;
  };

  const UsdMayaXformStack::IndexPair& opPair = UsdMayaXformStack::MayaStack().FindOpIndexPair(opName);

  // Add the second first, so that if insertAtBeginning is true, they will
  // maintain the same order
  auto secondPos = -1;
  if (opPair.second != UsdMayaXformStack::NO_INDEX)
  {
    secondPos = addOp(opPair.second, insertAtBeginning);
    if (secondPos == -1)
    {
      return MStatus::kFailure;
    }
  }
  auto firstPos = addOp(opPair.first, insertAtBeginning);
  if (firstPos == -1)
  {
    if (opPair.second != UsdMayaXformStack::NO_INDEX && secondPos != -1)
    {
      // Undo the insertion of the other pair if something went wrong
      m_orderedOps.erase(m_orderedOps.begin() + secondPos);
      m_xformops.erase(m_xformops.begin() + secondPos);
      m_orderedOpMayaIndices.erase(m_orderedOpMayaIndices.begin() + secondPos);

      assert(m_orderedOps.size() == m_xformops.size());
    }
    return MStatus::kFailure;
  }
  m_xform.SetXformOpOrder(m_xformops, (m_flags & kInheritsTransform) == 0);
  m_flags |= newFlag;
  return MStatus::kSuccess;
}

//----------------------------------------------------------------------------------------------------------------------
// Translation
//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertTranslateOp()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertTranslateOp\n");
  return insertOp(UsdGeomXformOp::TypeTranslate, UsdGeomXformOp::PrecisionFloat,
      UsdMayaXformStackTokens->translate, kPrimHasTranslation,
      // insertAtBeginning, because we know translate is always first in the stack,
      // so we can save a little time
      true);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::translateTo(const MVector& vector, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::translateTo %f %f %f\n", vector.x, vector.y, vector.z);
  if(isTranslateLocked())
    return MPxTransformationMatrix::translateTo(vector, space);

  MStatus status = MPxTransformationMatrix::translateTo(vector, space);
  if(status)
  {
    m_translationTweak = MPxTransformationMatrix::translationValue - m_translationFromUsd;
  }

  if(pushToPrimAvailable())
  {
    // if the prim does not contain a translation, make sure we insert a transform op for that.
    if(primHasTranslation())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix() && vector != MVector(0.0, 0.0, 0.0))
    {
      AL_MAYA_CHECK_ERROR(insertTranslateOp(), "error inserting Translate op");
    }
    pushTranslateToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
// Scale
//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertScaleOp()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertScaleOp\n");

  return insertOp(UsdGeomXformOp::TypeScale, UsdGeomXformOp::PrecisionFloat,
      UsdMayaXformStackTokens->scale, kPrimHasScale);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::scaleTo(const MVector& scale, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::scaleTo %f %f %f\n", scale.x, scale.y, scale.z);
  if(isScaleLocked())
    return MPxTransformationMatrix::scaleTo(scale, space);

  MStatus status = MPxTransformationMatrix::scaleTo(scale, space);
  if(status)
  {
    m_scaleTweak = MPxTransformationMatrix::scaleValue - m_scaleFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasScale())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix() && scale != MVector(1.0, 1.0, 1.0))
    {
      // rare case: add a new scale op into the prim
      AL_MAYA_CHECK_ERROR(insertScaleOp(), "error inserting Scale op");
    }
    pushScaleToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
// Shear
//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertShearOp()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertShearOp\n");
  return insertOp(UsdGeomXformOp::TypeTransform, UsdGeomXformOp::PrecisionDouble,
      UsdMayaXformStackTokens->shear, kPrimHasShear);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::shearTo(const MVector& shear, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::shearTo %f %f %f\n", shear.x, shear.y, shear.z);
  if(isShearLocked())
    return MPxTransformationMatrix::shearTo(shear, space);
  MStatus status = MPxTransformationMatrix::shearTo(shear, space);
  if(status)
  {
    m_shearTweak = MPxTransformationMatrix::shearValue - m_shearFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasShear())
    {
      // helping the branch predictor
    }
    else
    if(!pushPrimToMatrix() && shear != MVector(0.0, 0.0, 0.0))
    {
      // rare case: add a new scale op into the prim
      AL_MAYA_CHECK_ERROR(insertShearOp(), "error inserting Shear op");
    }
    pushShearToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertScalePivotOp()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertScalePivotOp\n");
  return insertOp(UsdGeomXformOp::TypeTranslate, UsdGeomXformOp::PrecisionFloat,
      UsdMayaXformStackTokens->scalePivot, kPrimHasScalePivot);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setScalePivot(const MPoint& sp, MSpace::Space space, bool balance)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setScalePivot %f %f %f\n", sp.x, sp.y, sp.z);
  MStatus status = MPxTransformationMatrix::setScalePivot(sp, space, balance);
  if(status)
  {
    m_scalePivotTweak = MPxTransformationMatrix::scalePivotValue - m_scalePivotFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasScalePivot())
    {
    }
    else
    if(!pushPrimToMatrix() && sp != MPoint(0.0, 0.0, 0.0, 1.0))
    {
      // We handle inserting and pushing in the same func, to deal with possible split
      // pivot
      return insertAndPushPivotOp(UsdMayaXformStackTokens->scalePivot);
    }
    pushScalePivotToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertScalePivotTranslationOp()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertScalePivotTranslationOp\n");
  return insertOp(UsdGeomXformOp::TypeTranslate, UsdGeomXformOp::PrecisionFloat,
      UsdMayaXformStackTokens->scalePivotTranslate, kPrimHasScalePivotTranslate);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setScalePivotTranslation(const MVector& sp, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setScalePivotTranslation %f %f %f\n", sp.x, sp.y, sp.z);
  MStatus status = MPxTransformationMatrix::setScalePivotTranslation(sp, space);
  if(status)
  {
    m_scalePivotTranslationTweak = MPxTransformationMatrix::scalePivotTranslationValue - m_scalePivotTranslationFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasScalePivotTranslate())
    {
    }
    else
    if(!pushPrimToMatrix() && sp != MVector(0.0, 0.0, 0.0))
    {
      AL_MAYA_CHECK_ERROR(insertScalePivotTranslationOp(), "error inserting ScalePivotTranslation op");
    }
    pushScalePivotTranslateToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertRotatePivotOp()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertRotatePivotOp\n");
  return insertOp(UsdGeomXformOp::TypeTranslate, UsdGeomXformOp::PrecisionFloat,
      UsdMayaXformStackTokens->rotatePivot, kPrimHasRotatePivot);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotatePivot(const MPoint& pivot, MSpace::Space space, bool balance)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setRotatePivot %f %f %f\n", pivot.x, pivot.y, pivot.z);
  MStatus status = MPxTransformationMatrix::setRotatePivot(pivot, space, balance);
  if(status)
  {
    m_rotatePivotTweak = MPxTransformationMatrix::rotatePivotValue - m_rotatePivotFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotatePivot())
    {
    }
    else
    if(!pushPrimToMatrix() && pivot != MPoint(0.0, 0.0, 0.0, 1.0))
    {
      // We handle inserting and pushing in the same func, to deal with possible split
      // pivot
      return insertAndPushPivotOp(UsdMayaXformStackTokens->rotatePivot);
    }
    pushRotatePivotToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertRotatePivotTranslationOp()
{
  return insertOp(UsdGeomXformOp::TypeTranslate, UsdGeomXformOp::PrecisionFloat,
      UsdMayaXformStackTokens->rotatePivotTranslate, kPrimHasRotatePivotTranslate);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotatePivotTranslation(const MVector &vector, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setRotatePivotTranslation %f %f %f\n", vector.x, vector.y, vector.z);
  MStatus status = MPxTransformationMatrix::setRotatePivotTranslation(vector, space);
  if(status)
  {
    m_rotatePivotTranslationTweak = MPxTransformationMatrix::rotatePivotTranslationValue - m_rotatePivotTranslationFromUsd;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotatePivotTranslate())
    {
    }
    else
    if(!pushPrimToMatrix() && vector != MPoint(0.0, 0.0, 0.0, 1.0))
    {
      AL_MAYA_CHECK_ERROR(insertRotatePivotTranslationOp(), "error inserting RotatePivotTranslation op");
    }
    pushRotatePivotTranslateToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertRotateOp()
{
  UsdGeomXformOp::Type opType;
  switch(rotationOrder())
  {
  case MTransformationMatrix::kXYZ:
    opType = UsdGeomXformOp::TypeRotateXYZ;
    break;

  case MTransformationMatrix::kXZY:
    opType = UsdGeomXformOp::TypeRotateXZY;
    break;

  case MTransformationMatrix::kYXZ:
    opType = UsdGeomXformOp::TypeRotateYXZ;
    break;

  case MTransformationMatrix::kYZX:
    opType = UsdGeomXformOp::TypeRotateYZX;
    break;

  case MTransformationMatrix::kZXY:
    opType = UsdGeomXformOp::TypeRotateZXY;
    break;

  case MTransformationMatrix::kZYX:
    opType = UsdGeomXformOp::TypeRotateZYX;
    break;

  default:
    TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::insertRotateOp - got invalid rotation order; assuming XYZ");
    opType = UsdGeomXformOp::TypeRotateXYZ;
    break;
  }

  return insertOp(opType, UsdGeomXformOp::PrecisionFloat,
      UsdMayaXformStackTokens->rotate, kPrimHasRotation);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::rotateTo(const MQuaternion &q, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::rotateTo %f %f %f %f\n", q.x, q.y, q.z, q.w);
  if(isRotateLocked())
    return MPxTransformationMatrix::rotateTo(q, space);;
  MStatus status = MPxTransformationMatrix::rotateTo(q, space);
  if(status)
  {
    m_rotationTweak.x = MPxTransformationMatrix::rotationValue.x - m_rotationFromUsd.x;
    m_rotationTweak.y = MPxTransformationMatrix::rotationValue.y - m_rotationFromUsd.y;
    m_rotationTweak.z = MPxTransformationMatrix::rotationValue.z - m_rotationFromUsd.z;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotation())
    {
    }
    else
    if(!pushPrimToMatrix() && q != MQuaternion(0.0, 0.0, 0.0, 1.0))
    {
      AL_MAYA_CHECK_ERROR(insertRotateOp(), "error inserting Rotate op");
    }
    pushRotateToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::rotateTo(const MEulerRotation &e, MSpace::Space space)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::rotateTo %f %f %f\n", e.x, e.y, e.z);
  if(isRotateLocked())
    return MPxTransformationMatrix::rotateTo(e, space);;
  MStatus status = MPxTransformationMatrix::rotateTo(e, space);
  if(status)
  {
    m_rotationTweak.x = MPxTransformationMatrix::rotationValue.x - m_rotationFromUsd.x;
    m_rotationTweak.y = MPxTransformationMatrix::rotationValue.y - m_rotationFromUsd.y;
    m_rotationTweak.z = MPxTransformationMatrix::rotationValue.z - m_rotationFromUsd.z;
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotation())
    {
    }
    else
    if(!pushPrimToMatrix() && e != MEulerRotation(0.0, 0.0, 0.0, MEulerRotation::kXYZ))
    {
      AL_MAYA_CHECK_ERROR(insertRotateOp(), "error inserting Rotate op");
    }
    pushRotateToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotationOrder(MTransformationMatrix::RotationOrder, bool)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setRotationOrder\n");
  // do not allow people to change the rotation order here.
  // It's too hard for my feeble brain to figure out how to remap that to the USD data.
  return MS::kFailure;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::insertRotateAxesOp()
{
  return insertOp(UsdGeomXformOp::TypeRotateXYZ, UsdGeomXformOp::PrecisionFloat,
      UsdMayaXformStackTokens->rotateAxis, kPrimHasRotateAxes);
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotateOrientation(const MQuaternion &q, MSpace::Space space, bool balance)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setRotateOrientation %f %f %f %f\n", q.x, q.y, q.z, q.w);
  MStatus status = MPxTransformationMatrix::setRotateOrientation(q, space, balance);
  if(status)
  {
    m_rotateOrientationFromUsd = MPxTransformationMatrix::rotateOrientationValue * m_rotateOrientationTweak.inverse();
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotateAxes())
    {
    }
    else
    if(!pushPrimToMatrix() && q != MQuaternion(0.0, 0.0, 0.0, 1.0))
    {
      AL_MAYA_CHECK_ERROR(insertRotateAxesOp(), "error inserting RotateAxes op");
    }
    pushRotateAxisToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
MStatus TransformationMatrix::setRotateOrientation(const MEulerRotation& euler, MSpace::Space space, bool balance)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::setRotateOrientation %f %f %f\n", euler.x, euler.y, euler.z);
  MStatus status = MPxTransformationMatrix::setRotateOrientation(euler, space, balance);
  if(status)
  {
    m_rotateOrientationFromUsd = MPxTransformationMatrix::rotateOrientationValue * m_rotateOrientationTweak.inverse();
  }
  if(pushToPrimAvailable())
  {
    if(primHasRotateAxes())
    {
    }
    else
    if(!pushPrimToMatrix() && euler != MEulerRotation(0.0, 0.0, 0.0, MEulerRotation::kXYZ))
    {
      AL_MAYA_CHECK_ERROR(insertRotateAxesOp(), "error inserting RotateAxes op");
    }
    pushRotateAxisToPrim();
  }
  return status;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::notifyProxyShapeOfRedraw()
{
  // Anytime we update the xform, we need to tell the proxy shape that it
  // needs to redraw itself
  MObject tn(m_transformNode.object());
  if (!tn.isNull())
  {
    MStatus status;
    MFnDependencyNode mfn(tn, &status);
    if (status && mfn.typeId() == Transform::kTypeId)
    {
      auto xform = static_cast<Transform*>(mfn.userNode());
      MObject proxyObj = xform->getProxyShape();
      if (!proxyObj.isNull())
      {
        MFnDependencyNode proxyMfn(proxyObj);
        if (proxyMfn.typeId() == ProxyShape::kTypeId)
        {
          GfMatrix4d oldMatrix;
          bool oldResetsStack;
          m_xform.GetLocalTransformation(&oldMatrix, &oldResetsStack, getTimeCode());

          // We check that the matrix actually HAS changed, as this function will be
          // called when, ie, pushToPrim is toggled, which often happens on node
          // creation, when nothing has actually changed
          GfMatrix4d newMatrix;
          bool newResetsStack;
          m_xform.GetLocalTransformation(&newMatrix, &newResetsStack, getTimeCode());
          if (newMatrix != oldMatrix || newResetsStack != oldResetsStack)
          {
            MHWRender::MRenderer::setGeometryDrawDirty(proxyObj);
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushTranslateToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushTranslateToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->translate)
    {
      UsdGeomXformOp& op = *it;
      internal_pushVector(MPxTransformationMatrix::translationValue, op);
      m_translationFromUsd = MPxTransformationMatrix::translationValue;
      m_translationTweak = MVector(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushPivotToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushPivotToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    const TfToken& opName = opIt->GetName();
    if(opName == UsdMayaXformStackTokens->pivot)
    {
      UsdGeomXformOp& op = *it;
      // is this a bug?
      internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
      m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
      m_rotatePivotTweak = MPoint(0, 0, 0);
      m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
      m_scalePivotTweak = MVector(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotatePivotToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotatePivotToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->rotatePivot)
    {
      UsdGeomXformOp& op = *it;
      internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
      m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
      m_rotatePivotTweak = MVector(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotatePivotTranslateToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotatePivotTranslateToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->rotatePivotTranslate)
    {
      UsdGeomXformOp& op = *it;
      internal_pushPoint(MPxTransformationMatrix::rotatePivotTranslationValue, op);
      m_rotatePivotTranslationFromUsd = MPxTransformationMatrix::rotatePivotTranslationValue;
      m_rotatePivotTranslationTweak = MVector(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotateToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotateToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->rotate)
    {
      UsdGeomXformOp& op = *it;
      internal_pushRotation(MPxTransformationMatrix::rotationValue, op);
      m_rotationFromUsd = MPxTransformationMatrix::rotationValue;
      m_rotationTweak = MEulerRotation(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushRotateAxisToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushRotateAxisToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->rotateAxis)
    {
      UsdGeomXformOp& op = *it;
      const double radToDeg = 180.0 / 3.141592654;
      MEulerRotation e = m_rotateOrientationFromUsd.asEulerRotation();
      MVector vec(e.x * radToDeg, e.y * radToDeg, e.z * radToDeg);
      internal_pushVector(vec, op);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushScalePivotTranslateToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushScalePivotTranslateToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->scalePivotTranslate)
    {
      UsdGeomXformOp& op = *it;
      internal_pushVector(MPxTransformationMatrix::scalePivotTranslationValue, op);
      m_scalePivotTranslationFromUsd = MPxTransformationMatrix::scalePivotTranslationValue;
      m_scalePivotTranslationTweak = MVector(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushScalePivotToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushScalePivotToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->scalePivot)
    {
      UsdGeomXformOp& op = *it;
      internal_pushPoint(MPxTransformationMatrix::scalePivotValue, op);
      m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
      m_scalePivotTweak = MPoint(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushScaleToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushScaleToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->scale)
    {
      UsdGeomXformOp& op = *it;
      internal_pushVector(MPxTransformationMatrix::scaleValue, op);
      m_scaleFromUsd = MPxTransformationMatrix::scaleValue;
      m_scaleTweak = MVector(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushShearToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushShearToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->shear)
    {
      UsdGeomXformOp& op = *it;
      internal_pushShear(MPxTransformationMatrix::shearValue, op);
      m_shearFromUsd = MPxTransformationMatrix::shearValue;
      m_shearTweak = MVector(0, 0, 0);
      return;
    }
  }
  // incase not found
  pushTransformToPrim();
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushTransformToPrim()
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushTransformToPrim\n");
  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    if(opIt->GetName() == UsdMayaXformStackTokens->transform)
    {
      UsdGeomXformOp& op = *it;
      if(pushPrimToMatrix())
      {
        internal_pushMatrix(asMatrix(), op);
      }
      return;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::pushToPrim()
{
  // if not yet intiaialised, do not execute this code! (It will crash!).
  if(!m_prim)
    return;
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::pushToPrim\n");

  GfMatrix4d oldMatrix;
  bool oldResetsStack;
  m_xform.GetLocalTransformation(&oldMatrix, &oldResetsStack, getTimeCode());

  auto opIt = m_orderedOps.begin();
  for(std::vector<UsdGeomXformOp>::iterator it = m_xformops.begin(), e = m_xformops.end(); it != e; ++it, ++opIt)
  {
    const UsdMayaXformOpClassification& opClass = *opIt;
    if (opClass.IsInvertedTwin()) continue;

    UsdGeomXformOp& op = *it;
    const TfToken& opName = opClass.GetName();
    if (opName == UsdMayaXformStackTokens->translate)
    {
      internal_pushVector(MPxTransformationMatrix::translationValue, op);
      m_translationFromUsd = MPxTransformationMatrix::translationValue;
      m_translationTweak = MVector(0, 0, 0);
    }
    else if (opName == UsdMayaXformStackTokens->pivot)
    {
      // is this a bug?
      internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
      m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
      m_rotatePivotTweak = MPoint(0, 0, 0);
      m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
      m_scalePivotTweak = MVector(0, 0, 0);
    }
    else if (opName == UsdMayaXformStackTokens->rotatePivotTranslate)
    {
      internal_pushPoint(MPxTransformationMatrix::rotatePivotTranslationValue, op);
      m_rotatePivotTranslationFromUsd = MPxTransformationMatrix::rotatePivotTranslationValue;
      m_rotatePivotTranslationTweak = MVector(0, 0, 0);
    }
    else if (opName == UsdMayaXformStackTokens->rotatePivot)
    {
      internal_pushPoint(MPxTransformationMatrix::rotatePivotValue, op);
      m_rotatePivotFromUsd = MPxTransformationMatrix::rotatePivotValue;
      m_rotatePivotTweak = MPoint(0, 0, 0);
    }
    else if (opName == UsdMayaXformStackTokens->rotate)
    {
      internal_pushRotation(MPxTransformationMatrix::rotationValue, op);
      m_rotationFromUsd = MPxTransformationMatrix::rotationValue;
      m_rotationTweak = MEulerRotation(0, 0, 0);
    }
    else if (opName == UsdMayaXformStackTokens->rotateAxis)
    {
      const double radToDeg = 180.0 / 3.141592654;
      MEulerRotation e = m_rotateOrientationFromUsd.asEulerRotation();
      MVector vec(e.x * radToDeg, e.y * radToDeg, e.z * radToDeg);
      internal_pushVector(vec, op);
    }
    else if (opName == UsdMayaXformStackTokens->scalePivotTranslate)
    {
      internal_pushVector(MPxTransformationMatrix::scalePivotTranslationValue, op);
      m_scalePivotTranslationFromUsd = MPxTransformationMatrix::scalePivotTranslationValue;
      m_scalePivotTranslationTweak = MVector(0, 0, 0);
    }

    else if (opName == UsdMayaXformStackTokens->scalePivot)
    {
      internal_pushPoint(MPxTransformationMatrix::scalePivotValue, op);
      m_scalePivotFromUsd = MPxTransformationMatrix::scalePivotValue;
      m_scalePivotTweak = MPoint(0, 0, 0);
    }
    else if (opName == UsdMayaXformStackTokens->shear)
    {
      internal_pushShear(MPxTransformationMatrix::shearValue, op);
      m_shearFromUsd = MPxTransformationMatrix::shearValue;
      m_shearTweak = MVector(0, 0, 0);
    }

    else if (opName == UsdMayaXformStackTokens->scale)
    {
      internal_pushVector(MPxTransformationMatrix::scaleValue, op);
      m_scaleFromUsd = MPxTransformationMatrix::scaleValue;
      m_scaleTweak = MVector(0, 0, 0);
    }
    else if (opName == UsdMayaXformStackTokens->transform)
    {
      if(pushPrimToMatrix())
      {
        internal_pushMatrix(asMatrix(), op);
      }
    }
  }
  notifyProxyShapeOfRedraw();
}

//----------------------------------------------------------------------------------------------------------------------
MMatrix TransformationMatrix::asMatrix() const
{
  MMatrix m = MPxTransformationMatrix::asMatrix();

  const double x = m_localTranslateOffset.x;
  const double y = m_localTranslateOffset.y;
  const double z = m_localTranslateOffset.z;

  m[3][0] += m[0][0] * x;
  m[3][1] += m[0][1] * x;
  m[3][2] += m[0][2] * x;
  m[3][0] += m[1][0] * y;
  m[3][1] += m[1][1] * y;
  m[3][2] += m[1][2] * y;
  m[3][0] += m[2][0] * z;
  m[3][1] += m[2][1] * z;
  m[3][2] += m[2][2] * z;

  // Let Maya know what the matrix should be
  return m;
}

//----------------------------------------------------------------------------------------------------------------------
MMatrix TransformationMatrix::asMatrix(double percent) const
{
  MMatrix m = MPxTransformationMatrix::asMatrix(percent);

  const double x = m_localTranslateOffset.x * percent;
  const double y = m_localTranslateOffset.y * percent;
  const double z = m_localTranslateOffset.z * percent;

  m[3][0] += m[0][0] * x;
  m[3][1] += m[0][1] * x;
  m[3][2] += m[0][2] * x;
  m[3][0] += m[1][0] * y;
  m[3][1] += m[1][1] * y;
  m[3][2] += m[1][2] * y;
  m[3][0] += m[2][0] * z;
  m[3][1] += m[2][1] * z;
  m[3][2] += m[2][2] * z;

  return m;
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::enableReadAnimatedValues(bool enabled)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::enableReadAnimatedValues\n");
  if(enabled) m_flags |= kReadAnimatedValues;
  else m_flags &= ~kReadAnimatedValues;

  // if not yet intiaialised, do not execute this code! (It will crash!).
  if(!m_prim)
    return;

  // if we are enabling push to prim, we need to see if anything has changed on the transform since the last time
  // the values were synced. I'm assuming that if a given transform attribute is not the same as the default, or
  // the prim already has a transform op for that attribute, then just call a method to make a minor adjustment
  // of nothing. This will call my code that will magically construct the transform ops in the right order.
  if(enabled)
  {
    const MVector nullVec(0, 0, 0);
    const MVector oneVec(1.0, 1.0, 1.0);
    const MPoint nullPoint(0, 0, 0);
    const MQuaternion nullQuat(0, 0, 0, 1.0);

    if(!pushPrimToMatrix())
    {
      if(primHasTranslation() || translation() != nullVec)
        translateBy(nullVec);

      if(primHasScale() || scale() != oneVec)
        scaleBy(oneVec);

      if(primHasShear() || shear() != nullVec)
        shearBy(nullVec);

      if(primHasScalePivot() || scalePivot() != nullPoint)
        setScalePivot(scalePivot(), MSpace::kTransform, false);

      if(primHasScalePivotTranslate() || scalePivotTranslation() != nullVec)
        setScalePivotTranslation(scalePivotTranslation(), MSpace::kTransform);

      if(primHasRotatePivot() || rotatePivot() != nullPoint)
        setRotatePivot(rotatePivot(), MSpace::kTransform, false);

      if(primHasRotatePivotTranslate() || rotatePivotTranslation() != nullVec)
        setRotatePivotTranslation(rotatePivotTranslation(), MSpace::kTransform);

      if(primHasRotation() || rotation() != nullQuat)
        rotateBy(nullQuat);

      if(primHasRotateAxes() || rotateOrientation() != nullQuat)
        setRotateOrientation(rotateOrientation(), MSpace::kTransform, false);
    }
    else
    if(primHasTransform())
    {
      for(size_t i = 0, n = m_orderedOps.size(); i < n; ++i)
      {
        if(m_orderedOps[i].GetName() == UsdMayaXformStackTokens->transform)
        {
          internal_pushMatrix(asMatrix(), m_xformops[i]);
          break;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
void TransformationMatrix::enablePushToPrim(bool enabled)
{
  TF_DEBUG(ALUSDMAYA_TRANSFORM_MATRIX).Msg("TransformationMatrix::enablePushToPrim\n");
  if(enabled) m_flags |= kPushToPrimEnabled;
  else m_flags &= ~kPushToPrimEnabled;

  // if not yet intiaialised, do not execute this code! (It will crash!).
  if(!m_prim)
    return;

  // if we are enabling push to prim, we need to see if anything has changed on the transform since the last time
  // the values were synced. I'm assuming that if a given transform attribute is not the same as the default, or
  // the prim already has a transform op for that attribute, then just call a method to make a minor adjustment
  // of nothing. This will call my code that will magically construct the transform ops in the right order.
  if(enabled && getTimeCode() == UsdTimeCode::Default())
  {
    const MVector nullVec(0, 0, 0);
    const MVector oneVec(1.0, 1.0, 1.0);
    const MPoint nullPoint(0, 0, 0);
    const MQuaternion nullQuat(0, 0, 0, 1.0);

    if(!pushPrimToMatrix())
    {
      if(primHasTranslation() || translation() != nullVec)
        translateTo(translation());

      if(primHasScale() || scale() != oneVec)
        scaleTo(scale());

      if(primHasShear() || shear() != nullVec)
        shearTo(shear());

      if(primHasScalePivot() || scalePivot() != nullPoint)
        setScalePivot(scalePivot(), MSpace::kTransform, false);

      if(primHasScalePivotTranslate() || scalePivotTranslation() != nullVec)
        setScalePivotTranslation(scalePivotTranslation(), MSpace::kTransform);

      if(primHasRotatePivot() || rotatePivot() != nullPoint)
        setRotatePivot(rotatePivot(), MSpace::kTransform, false);

      if(primHasRotatePivotTranslate() || rotatePivotTranslation() != nullVec)
        setRotatePivotTranslation(rotatePivotTranslation(), MSpace::kTransform);

      if(primHasRotation() || rotation() != nullQuat)
        rotateTo(rotation());

      if(primHasRotateAxes() || rotateOrientation() != nullQuat)
        setRotateOrientation(rotateOrientation(), MSpace::kTransform, false);
    }
    else
    if(primHasTransform())
    {
      for(size_t i = 0, n = m_orderedOps.size(); i < n; ++i)
      {
        if(m_orderedOps[i].GetName() == UsdMayaXformStackTokens->transform)
        {
          internal_pushMatrix(asMatrix(), m_xformops[i]);
          break;
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------
} // nodes
} // usdmaya
} // AL
//----------------------------------------------------------------------------------------------------------------------
