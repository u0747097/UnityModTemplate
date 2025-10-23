#pragma once

#include "enums.h"
#include "cs-types.h"
#include "types-helper.h"

using UTYPE = UnityResolve::UnityType;

///////////////////////////////
/// All Forward Declarations
///////////////////////////////

class Application;


///////////////////////////////
/// Class Definitions
///////////////////////////////

class Application
{
    UNITY_CLASS_DECL("UnityEngine.CoreModule.dll", "Application")

    UNITY_METHOD(UTYPE::String*, get_version)
};
