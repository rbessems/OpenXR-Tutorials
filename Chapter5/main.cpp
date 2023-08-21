// Copyright Khronos Group 2023
// OpenXR Tutorial for Khronos Group

#include "DebugOutput.h"
// XR_DOCS_TAG_BEGIN_include_GraphicsAPIs
#include "GraphicsAPI_D3D11.h"
#include "GraphicsAPI_D3D12.h"
#include "GraphicsAPI_OpenGL.h"
#include "GraphicsAPI_OpenGL_ES.h"
#include "GraphicsAPI_Vulkan.h"
// XR_DOCS_TAG_END_include_GraphicsAPIs
#include "OpenXRDebugUtils.h"

// XR_DOCS_TAG_BEGIN_DeclareExtensionFunctions
PFN_xrCreateHandTrackerEXT xrCreateHandTrackerEXT=nullptr;
PFN_xrDestroyHandTrackerEXT xrDestroyHandTrackerEXT=nullptr;
PFN_xrLocateHandJointsEXT xrLocateHandJointsEXT=nullptr;
// XR_DOCS_TAG_END_DeclareExtensionFunctions

// XR_DOCS_TAG_BEGIN_include_linear_algebra
// include xr linear algebra for XrVector and XrMatrix classes.
#include "xr_linear_algebra.h"
// Declare some useful operators for vectors:
XrVector3f operator-(XrVector3f a,XrVector3f b)
{
    return {a.x-b.x,a.y-b.y,a.z-b.z};
}
XrVector3f operator*(XrVector3f a,float b)
{
    return {a.x*b,a.y*b,a.z*b};
}
// Include <algorithm> for std::min and max
#include <algorithm> 
// Random numbers for colourful blocks
#include <random>
static std::uniform_real_distribution<float> pseudorandom_distribution(0,1.f);
static std::mt19937 pseudo_random_generator;
// XR_DOCS_TAG_END_include_linear_algebra

#if defined(__ANDROID__)
// XR_DOCS_TAG_BEGIN_ReadFiles_Android
#include "android/asset_manager.h"
std::string ReadTextFile(const std::string &filepath, AAssetManager *assetManager) {
    AAsset *file = AAssetManager_open(assetManager, filepath.c_str(), AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);
    std::string text;
    text.resize(fileLength);
    AAsset_read(file, (void *)text.data(), fileLength);
    AAsset_close(file);
    return text;
}
std::vector<char> ReadBinaryFile(const std::string &filepath, AAssetManager *assetManager) {
    AAsset *file = AAssetManager_open(assetManager, filepath.c_str(), AASSET_MODE_BUFFER);
    size_t fileLength = AAsset_getLength(file);
    std::vector<char> binary(fileLength);
    AAsset_read(file, (void *)binary.data(), fileLength);
    AAsset_close(file);
    return binary;
}
// XR_DOCS_TAG_END_ReadFiles_Android
#endif
#define XR_DOCS_CHAPTER_VERSION XR_DOCS_CHAPTER_4_5

class OpenXRTutorial {
public:
    OpenXRTutorial(GraphicsAPI_Type api)
        : m_apiType(api) {
        if (!CheckGraphicsAPI_TypeIsValidForPlatform(m_apiType)) {
            std::cout << "ERROR: The provided Graphics API is not valid for this platform." << std::endl;
            DEBUG_BREAK;
        }
    }
    ~OpenXRTutorial() = default;

    void Run() {
        CreateInstance();
        CreateDebugMessenger();

        GetInstanceProperties();
        GetSystemID();
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_4_1
// XR_DOCS_TAG_BEGIN_CallCreateActionSet
        CreateActionSet();
// XR_DOCS_TAG_END_CallCreateActionSet
// XR_DOCS_TAG_BEGIN_CallSuggestBindings
        SuggestBindings();
// XR_DOCS_TAG_END_CallSuggestBindings
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_1
        GetViewConfigurationViews();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_2
        GetEnvironmentBlendModes();
#endif

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_2_2
        CreateSession();

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_4_3
// XR_DOCS_TAG_BEGIN_CallCreateActionPoses
        CreateActionPoses();
        AttachActionSet();
// XR_DOCS_TAG_END_CallCreateActionPoses
#endif
// XR_DOCS_TAG_BEGIN_CallCreateHandTracker
	if(handTrackingSystemProperties.supportsHandTracking)
		CreateHandTrackers();
// XR_DOCS_TAG_END_CallCreateHandTracker

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_2
        CreateReferenceSpace();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_1
        CreateSwapchain();
#endif
#endif
        CreateResources();

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_2_3
        while (m_applicationRunning) {
            PollSystemEvents();
            PollEvents();
            if (m_sessionRunning) {
                RenderFrame();
            }
        }
#endif

#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_1
        DestroySwapchain();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_2
        DestroyReferenceSpace();
#endif
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_2_2
        // XR_DOCS_TAG_BEGIN_CallDestroyResources
        DestroyResources();
        // XR_DOCS_TAG_END_CallDestroyResources
        DestroySession();
#endif

        DestroyDebugMessenger();
        DestroyInstance();
    }

private:
    void CreateInstance() {
        // XR_DOCS_TAG_BEGIN_XrApplicationInfo
        XrApplicationInfo AI;
        strncpy(AI.applicationName, "OpenXR Tutorial Chapter 4", XR_MAX_APPLICATION_NAME_SIZE);
        AI.applicationVersion = 1;
        strncpy(AI.engineName, "OpenXR Engine", XR_MAX_ENGINE_NAME_SIZE);
        AI.engineVersion = 1;
        AI.apiVersion = XR_CURRENT_API_VERSION;
        // XR_DOCS_TAG_END_XrApplicationInfo

        // Add additional instance layers/extensions
        {
            // XR_DOCS_TAG_BEGIN_instanceExtensions
            m_instanceExtensions.push_back(XR_EXT_DEBUG_UTILS_EXTENSION_NAME);
            m_instanceExtensions.push_back(GetGraphicsAPIInstanceExtensionString(m_apiType));
            // XR_DOCS_TAG_END_instanceExtensions
            // XR_DOCS_TAG_BEGIN_handTrackingExtensions
            m_instanceExtensions.push_back(XR_EXT_HAND_TRACKING_EXTENSION_NAME);
            // XR_DOCS_TAG_END_handTrackingExtensions
        }

        // XR_DOCS_TAG_BEGIN_find_apiLayer_extension
        uint32_t apiLayerCount = 0;
        std::vector<XrApiLayerProperties> apiLayerProperties;
        OPENXR_CHECK(xrEnumerateApiLayerProperties(0, &apiLayerCount, nullptr), "Failed to enumerate ApiLayerProperties.");
        apiLayerProperties.resize(apiLayerCount, {XR_TYPE_API_LAYER_PROPERTIES});

        OPENXR_CHECK(xrEnumerateApiLayerProperties(apiLayerCount, &apiLayerCount, apiLayerProperties.data()), "Failed to enumerate ApiLayerProperties.");
        for (auto &requestLayer : m_apiLayers) {
            for (auto &layerProperty : apiLayerProperties) {
                if (strcmp(requestLayer.c_str(), layerProperty.layerName)) {
                    continue;
                } else {
                    m_activeAPILayers.push_back(requestLayer.c_str());
                    break;
                }
            }
        }

        uint32_t extensionCount = 0;
        std::vector<XrExtensionProperties> extensionProperties;
        OPENXR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr), "Failed to enumerate InstanceExtensionProperties.");
        extensionProperties.resize(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});

        OPENXR_CHECK(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data()), "Failed to enumerate InstanceExtensionProperties.");
        for (auto &requestExtension : m_instanceExtensions) {
            bool found = false;
            for (auto &extensionProperty : extensionProperties) {
                if (strcmp(requestExtension.c_str(), extensionProperty.extensionName)) {
                    continue;
                } else {
                    m_activeInstanceExtensions.push_back(requestExtension.c_str());
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::cerr << "Failed to find OpenXR instance extension: " << requestExtension << "\n";
            }
        }
        // XR_DOCS_TAG_END_find_apiLayer_extension

        // XR_DOCS_TAG_BEGIN_XrInstanceCreateInfo
        XrInstanceCreateInfo instanceCI{XR_TYPE_INSTANCE_CREATE_INFO};
        instanceCI.createFlags = 0;
        instanceCI.applicationInfo = AI;
        instanceCI.enabledApiLayerCount = static_cast<uint32_t>(m_activeAPILayers.size());
        instanceCI.enabledApiLayerNames = m_activeAPILayers.data();
        instanceCI.enabledExtensionCount = static_cast<uint32_t>(m_activeInstanceExtensions.size());
        instanceCI.enabledExtensionNames = m_activeInstanceExtensions.data();
        OPENXR_CHECK(xrCreateInstance(&instanceCI, &m_xrInstance), "Failed to create Instance.");
        // XR_DOCS_TAG_END_XrInstanceCreateInfo

		// XR_DOCS_TAG_BEGIN_ExtensionFunctions
		OPENXR_CHECK(xrGetInstanceProcAddr(m_xrInstance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction *)&xrCreateHandTrackerEXT), "Failed to get xrCreateHandTrackerEXT.");
		OPENXR_CHECK(xrGetInstanceProcAddr(m_xrInstance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction *)&xrDestroyHandTrackerEXT), "Failed to get xrDestroyHandTrackerEXT.");
		OPENXR_CHECK(xrGetInstanceProcAddr(m_xrInstance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction *)&xrLocateHandJointsEXT), "Failed to get xrLocateHandJointsEXT.");
		// XR_DOCS_TAG_BEGIN_ExtensionFunctions
    }

    void DestroyInstance() {
        OPENXR_CHECK(xrDestroyInstance(m_xrInstance), "Failed to destroy Instance.");
    }

    // XR_DOCS_TAG_BEGIN_Create_DestroyDebugMessenger
    void CreateDebugMessenger() {
        if (IsStringInVector(m_activeInstanceExtensions, XR_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            m_debugUtilsMessenger = CreateOpenXRDebugUtilsMessenger(m_xrInstance);
        }
    }
    void DestroyDebugMessenger() {
        if (IsStringInVector(m_activeInstanceExtensions, XR_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
            DestroyOpenXRDebugUtilsMessenger(m_xrInstance, m_debugUtilsMessenger);
        }
    }
    // XR_DOCS_TAG_END_Create_DestroyDebugMessenger

    // XR_DOCS_TAG_BEGIN_GetInstanceProperties
    void GetInstanceProperties() {
        XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
        OPENXR_CHECK(xrGetInstanceProperties(m_xrInstance, &instanceProperties), "Failed to get InstanceProperties.");

        std::cout << "OpenXR Runtime: " << instanceProperties.runtimeName << " - ";
        std::cout << XR_VERSION_MAJOR(instanceProperties.runtimeVersion) << ".";
        std::cout << XR_VERSION_MINOR(instanceProperties.runtimeVersion) << ".";
        std::cout << XR_VERSION_PATCH(instanceProperties.runtimeVersion) << std::endl;
    }
    // XR_DOCS_TAG_END_GetInstanceProperties

    // XR_DOCS_TAG_BEGIN_GetSystemID
    void GetSystemID() {
        XrSystemGetInfo systemGI{XR_TYPE_SYSTEM_GET_INFO};
        systemGI.formFactor = m_formFactor;
        OPENXR_CHECK(xrGetSystem(m_xrInstance, &systemGI, &m_systemID), "Failed to get SystemID.");
// XR_DOCS_TAG_BEGIN_SystemHandTrackingProperties
		systemProperties.next=&handTrackingSystemProperties;
// XR_DOCS_TAG_END_SystemHandTrackingProperties
        OPENXR_CHECK(xrGetSystemProperties(m_xrInstance, m_systemID, &systemProperties), "Failed to get SystemProperties.");
    }
    // XR_DOCS_TAG_END_GetSystemID
    
    // XR_DOCS_TAG_BEGIN_CreateXrPath
    XrPath CreateXrPath(const char *path_string) {
        XrPath xrPath;
        OPENXR_CHECK(xrStringToPath(m_xrInstance, path_string, &xrPath), "Failed to create XrPath from string.");
        return xrPath;
    }
    std::string FromXrPath(XrPath path) {
        uint32_t strl;
        char text[XR_MAX_PATH_LENGTH];
        XrResult res;
        res = xrPathToString(m_xrInstance, path, XR_MAX_PATH_LENGTH, &strl, text);
        std::string str;
        if (res == XR_SUCCESS) {
            str = text;
        } else {
            OPENXR_CHECK(res, "Failed to retrieve path.");
        }
        return str;
    }
    // XR_DOCS_TAG_END_CreateXrPath
    // XR_DOCS_TAG_BEGIN_CreateActionSet
    void CreateActionSet() {
        XrActionSetCreateInfo actionSetCI{XR_TYPE_ACTION_SET_CREATE_INFO};
        strncpy(actionSetCI.actionSetName, "openxr-tutorial-actionset", XR_MAX_ACTION_SET_NAME_SIZE);
        strncpy(actionSetCI.localizedActionSetName, "OpenXR Tutorial ActionSet", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE);
        OPENXR_CHECK(xrCreateActionSet(m_xrInstance, &actionSetCI, &m_actionSet), "Failed to create ActionSet.");
        // XR_DOCS_TAG_END_CreateActionSet

        // XR_DOCS_TAG_BEGIN_CreateActions
        auto CreateAction = [this](XrAction &xrAction, const char *name, XrActionType xrActionType, std::vector<const char *> subaction_paths = {}) -> void {
            XrActionCreateInfo actionCI{XR_TYPE_ACTION_CREATE_INFO};
            actionCI.actionType = xrActionType;
            std::vector<XrPath> subaction_xrpaths;
            for (auto p : subaction_paths) {
                subaction_xrpaths.push_back(CreateXrPath(p));
            }
            actionCI.countSubactionPaths = (uint32_t)subaction_xrpaths.size();
            actionCI.subactionPaths = subaction_xrpaths.data();
            strncpy(actionCI.actionName, name, XR_MAX_ACTION_NAME_SIZE);
            strncpy(actionCI.localizedActionName, name, XR_MAX_LOCALIZED_ACTION_NAME_SIZE);
            OPENXR_CHECK(xrCreateAction(m_actionSet, &actionCI, &xrAction), "Failed to create Action.");
        };
        CreateAction(m_grabAction, "grab", XR_ACTION_TYPE_FLOAT_INPUT, {"/user/hand/left", "/user/hand/right"});
        CreateAction(m_controllerGripPoseAction, "controller-grip", XR_ACTION_TYPE_POSE_INPUT, {"/user/hand/left", "/user/hand/right"});
        CreateAction(m_buzzAction, "buzz", XR_ACTION_TYPE_VIBRATION_OUTPUT, {"/user/hand/left", "/user/hand/right"});
        m_handPaths[0]=CreateXrPath("/user/hand/left");
        m_handPaths[1]=CreateXrPath("/user/hand/right");
    }
    // XR_DOCS_TAG_END_CreateActions

    // XR_DOCS_TAG_BEGIN_SuggestBindings1
    void SuggestBindings() {
        auto SuggestBindings = [this](const char *profile_path, std::vector<XrActionSuggestedBinding> bindings) -> bool {
            // The application can call xrSuggestInteractionProfileBindings once per interaction profile that it supports.
            XrInteractionProfileSuggestedBinding interactionProfileSuggestedBinding{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
            interactionProfileSuggestedBinding.interactionProfile = CreateXrPath(profile_path);
            interactionProfileSuggestedBinding.suggestedBindings = bindings.data();
            interactionProfileSuggestedBinding.countSuggestedBindings = (uint32_t)bindings.size();
            if (xrSuggestInteractionProfileBindings(m_xrInstance, &interactionProfileSuggestedBinding) == XrResult::XR_SUCCESS)
                return true;
            std::cout << "Failed to suggest bindings with " << profile_path << "\n";
            return false;
        };
        // XR_DOCS_TAG_END_SuggestBindings1
        // XR_DOCS_TAG_BEGIN_SuggestBindings2
        bool any_ok = false;
        any_ok |= SuggestBindings("/interaction_profiles/khr/simple_controller", {{m_grabAction                    , CreateXrPath("/user/hand/left/input/select/click")},
                                                                                  {m_grabAction                    , CreateXrPath("/user/hand/right/input/select/click")},
                                                                                  {m_controllerGripPoseAction    , CreateXrPath("/user/hand/left/input/grip/pose")},
                                                                                  {m_controllerGripPoseAction    , CreateXrPath("/user/hand/right/input/grip/pose")},
                                                                                  {m_buzzAction                    , CreateXrPath("/user/hand/left/output/haptic")},
                                                                                  {m_buzzAction                    , CreateXrPath("/user/hand/right/output/haptic")}
        });
// XR_DOCS_TAG_END_SuggestBindings2
// XR_DOCS_TAG_BEGIN_SuggestTouchNativeBindings
        any_ok |= SuggestBindings("/interaction_profiles/oculus/touch_controller", {{m_grabAction                , CreateXrPath("/user/hand/left/input/squeeze/value")},
                                                                                    {m_grabAction                , CreateXrPath("/user/hand/right/input/squeeze/value")},
                                                                                    {m_controllerGripPoseAction  , CreateXrPath("/user/hand/left/input/grip/pose")},
                                                                                    {m_controllerGripPoseAction  , CreateXrPath("/user/hand/right/input/grip/pose")},
                                                                                    {m_buzzAction                , CreateXrPath("/user/hand/left/output/haptic")},
                                                                                    {m_buzzAction                , CreateXrPath("/user/hand/right/output/haptic")}});
// XR_DOCS_TAG_BEGIN_SuggestTouchNativeBindings
// XR_DOCS_TAG_BEGIN_SuggestBindings3
        if (!any_ok) {
            DEBUG_BREAK;
        }
    }
    void RecordCurrentBindings() {
        if(m_session)
        {
        // now we are ready to:
            XrInteractionProfileState interactionProfile={XR_TYPE_INTERACTION_PROFILE_STATE,0,0};
            // for each action, what is the binding?
            OPENXR_CHECK(xrGetCurrentInteractionProfile(m_session,m_handPaths[0],&interactionProfile),"Failed to get profile.");
            if(interactionProfile.interactionProfile)
                std::cout<<" user/hand/left ActiveProfile "<<FromXrPath(interactionProfile.interactionProfile).c_str()<<std::endl;
            OPENXR_CHECK(xrGetCurrentInteractionProfile(m_session,m_handPaths[1],&interactionProfile),"Failed to get profile.");
            if(interactionProfile.interactionProfile)
                std::cout<<"user/hand/right ActiveProfile "<<FromXrPath(interactionProfile.interactionProfile).c_str()<<std::endl;
        }
    }
// XR_DOCS_TAG_END_SuggestBindings3
// XR_DOCS_TAG_BEGIN_CreateActionPoses
    void CreateActionPoses() {
        // Create an xrSpace for a pose action.
        auto CreateActionPoseSpace = [this](XrSession session, XrAction xrAction,const char *subaction_path=nullptr) -> XrSpace {
            XrSpace xrSpace;
            const XrPosef xrPoseIdentity = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
            // Create frame of reference for a pose action
            XrActionSpaceCreateInfo actionSpaceCI{XR_TYPE_ACTION_SPACE_CREATE_INFO};
            actionSpaceCI.action = xrAction;
            actionSpaceCI.poseInActionSpace = xrPoseIdentity;
            if(subaction_path)
                actionSpaceCI.subactionPath=CreateXrPath(subaction_path);
            OPENXR_CHECK(xrCreateActionSpace(session, &actionSpaceCI, &xrSpace), "Failed to create ActionSpace.");
            return xrSpace;
        };
        m_controllerGripPoseSpace[0] = CreateActionPoseSpace(m_session, m_controllerGripPoseAction,"/user/hand/left");
        m_controllerGripPoseSpace[1] = CreateActionPoseSpace(m_session, m_controllerGripPoseAction,"/user/hand/right");
    }
// XR_DOCS_TAG_END_CreateActionPoses
// XR_DOCS_TAG_BEGIN_AttachActionSet
    void AttachActionSet() {
        // Attach the action set we just made to the session. We could attach multiple action sets!
        XrSessionActionSetsAttachInfo actionSetAttachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        actionSetAttachInfo.countActionSets = 1;
        actionSetAttachInfo.actionSets = &m_actionSet;
        OPENXR_CHECK(xrAttachSessionActionSets(m_session, &actionSetAttachInfo), "Failed to attach ActionSet to Session.");
    }
// XR_DOCS_TAG_END_AttachActionSet
// XR_DOCS_TAG_BEGIN_CreateHandTracker
    void CreateHandTrackers() {
        for(int i=0;i<2;i++) {
			Hand &hand=m_hands[i];
            XrHandTrackerCreateInfoEXT xrHandTrackerCreateInfo={XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
            xrHandTrackerCreateInfo.hand = i==0?XR_HAND_LEFT_EXT:XR_HAND_RIGHT_EXT;
            xrHandTrackerCreateInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
            OPENXR_CHECK( xrCreateHandTrackerEXT(m_session,&xrHandTrackerCreateInfo,&hand.m_handTracker), "Failed to create Hand Tracker.");
        }
    }
// XR_DOCS_TAG_END_CreateHandTracker

// XR_DOCS_TAG_BEGIN_GetEnvironmentBlendModes
void GetEnvironmentBlendModes() {
    uint32_t environmentBlendModeSize = 0;
    OPENXR_CHECK(xrEnumerateEnvironmentBlendModes(m_xrInstance, m_systemID, m_viewConfiguration, 0, &environmentBlendModeSize, nullptr), "Failed to enumerate EnvironmentBlend Modes.");
    m_environmentBlendModes.resize(environmentBlendModeSize);
    OPENXR_CHECK(xrEnumerateEnvironmentBlendModes(m_xrInstance, m_systemID, m_viewConfiguration, environmentBlendModeSize, &environmentBlendModeSize, m_environmentBlendModes.data()), "Failed to enumerate EnvironmentBlend Modes.");

        m_environmentBlendMode = m_environmentBlendModes[0];
        // Pick the first application supported blend mode supported by the hardware.
        for (const XrEnvironmentBlendMode &environmentBlendMode : m_applicationEnvironmentBlendModes) {
            if (std::find(m_environmentBlendModes.begin(), m_environmentBlendModes.end(), environmentBlendMode) != m_environmentBlendModes.end()) {
                m_environmentBlendMode = environmentBlendMode;
                break;
            }
        }
    }
// XR_DOCS_TAG_END_GetEnvironmentBlendModes
    // XR_DOCS_TAG_BEGIN_GetViewConfigurationViews
    void GetViewConfigurationViews() {
        uint32_t viewConfigurationViewSize = 0;
        OPENXR_CHECK(xrEnumerateViewConfigurationViews(m_xrInstance, m_systemID, m_viewConfiguration, 0, &viewConfigurationViewSize, nullptr), "Failed to enumerate ViewConfigurationViews.");
        m_viewConfigurationViews.resize(viewConfigurationViewSize, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
        OPENXR_CHECK(xrEnumerateViewConfigurationViews(m_xrInstance, m_systemID, m_viewConfiguration, viewConfigurationViewSize, &viewConfigurationViewSize, m_viewConfigurationViews.data()), "Failed to enumerate ViewConfigurationViews.");
    }
    // XR_DOCS_TAG_END_GetViewConfigurationViews

    // XR_DOCS_TAG_BEGIN_CreateDestroySession
    void CreateSession() {
        XrSessionCreateInfo sessionCI{XR_TYPE_SESSION_CREATE_INFO};

        if (m_apiType == D3D11) {
#if defined(XR_USE_GRAPHICS_API_D3D11)
            m_graphicsAPI = std::make_unique<GraphicsAPI_D3D11>(m_xrInstance, m_systemID);
#endif
        } else if (m_apiType == D3D12) {
#if defined(XR_USE_GRAPHICS_API_D3D12)
            m_graphicsAPI = std::make_unique<GraphicsAPI_D3D12>(m_xrInstance, m_systemID);
#endif
        } else if (m_apiType == OPENGL) {
#if defined(XR_USE_GRAPHICS_API_OPENGL)
            m_graphicsAPI = std::make_unique<GraphicsAPI_OpenGL>(m_xrInstance, m_systemID);
#endif
        } else if (m_apiType == OPENGL_ES) {
#if defined(XR_USE_GRAPHICS_API_OPENGL_ES)
            m_graphicsAPI = std::make_unique<GraphicsAPI_OpenGL_ES>(m_xrInstance, m_systemID);
#endif
        } else if (m_apiType == VULKAN) {
#if defined(XR_USE_GRAPHICS_API_VULKAN)
            m_graphicsAPI = std::make_unique<GraphicsAPI_Vulkan>(m_xrInstance, m_systemID);
#endif
        } else {
            std::cout << "ERROR: Unknown Graphics API." << std::endl;
            DEBUG_BREAK;
        }
        sessionCI.next = m_graphicsAPI->GetGraphicsBinding();
        sessionCI.createFlags = 0;
        sessionCI.systemId = m_systemID;

        OPENXR_CHECK(xrCreateSession(m_xrInstance, &sessionCI, &m_session), "Failed to create Session.");
    }

    void DestroySession() {
// XR_DOCS_TAG_BEGIN_DestroyHandTracker
		for(int i=0;i<2;i++)
        {
			xrDestroyHandTrackerEXT(m_hands[i].m_handTracker);
		}
// XR_DOCS_TAG_END_DestroyHandTracker
        OPENXR_CHECK(xrDestroySession(m_session), "Failed to destroy Session.");
    }
    // XR_DOCS_TAG_END_CreateDestroySession
    // XR_DOCS_TAG_BEGIN_CreateResources1
    struct CameraConstants {
        XrMatrix4x4f viewProj;
        XrMatrix4x4f modelViewProj;
        XrMatrix4x4f model;
        XrVector4f colour;
        XrVector4f pad1;
        XrVector4f pad2;
        XrVector4f pad3;
    };
    CameraConstants cameraConstants;
        XrVector4f normals[6] = {
            {1.00f , 0.00f    , 0.00f,0},
            {-1.00f, 0.00f    , 0.00f,0},
            {0.00f , 1.00f    , 0.00f,0},
            {0.00f , -1.00f    , 0.00f,0},
            {0.00f , 0.00f    , 1.00f,0},
            {0.00f , 0.0f    ,-1.00f,0}};

    void CreateResources() {
        // Vertices for a 1x1x1 meter cube. (Left/Right, Top/Bottom, Front/Back)
        constexpr XrVector4f vertexPositions[] = {
            {+0.5f, +0.5f, +0.5f, 1.0f},
            {+0.5f, +0.5f, -0.5f, 1.0f},
            {+0.5f, -0.5f, +0.5f, 1.0f},
            {+0.5f, -0.5f, -0.5f, 1.0f},
            {-0.5f, +0.5f, +0.5f, 1.0f},
            {-0.5f, +0.5f, -0.5f, 1.0f},
            {-0.5f, -0.5f, +0.5f, 1.0f},
            {-0.5f, -0.5f, -0.5f, 1.0f}};

#define CUBE_FACE(V1, V2, V3, V4, V5, V6) vertexPositions[V1], vertexPositions[V2], vertexPositions[V3], vertexPositions[V4], vertexPositions[V5], vertexPositions[V6],

        XrVector4f cubeVertices[] = {
            CUBE_FACE(2, 1, 0, 2, 3, 1)  // -X
            CUBE_FACE(6, 4, 5, 6, 5, 7)  // +X
            CUBE_FACE(0, 1, 5, 0, 5, 4)  // -Y
            CUBE_FACE(2, 6, 7, 2, 7, 3)  // +Y
            CUBE_FACE(0, 4, 6, 0, 6, 2)  // -Z
            CUBE_FACE(1, 3, 7, 1, 7, 5)  // +Z
        };

        uint32_t cubeIndices[36] = {
            0, 1, 2, 3, 4, 5,        // -X
            6, 7, 8, 9, 10, 11,      // +X
            12, 13, 14, 15, 16, 17,  // -Y
            18, 19, 20, 21, 22, 23,  // +Y
            24, 25, 26, 27, 28, 29,  // -Z
            30, 31, 32, 33, 34, 35,  // +Z
        };
        m_vertexBuffer = m_graphicsAPI->CreateBuffer({GraphicsAPI::BufferCreateInfo::Type::VERTEX, sizeof(float) * 4, sizeof(cubeVertices), &cubeVertices});

        m_indexBuffer = m_graphicsAPI->CreateBuffer({GraphicsAPI::BufferCreateInfo::Type::INDEX, sizeof(uint32_t), sizeof(cubeIndices), &cubeIndices});

        size_t numberOfCuboids = 64 + 2 + 2;
// XR_DOCS_TAG_BEGIN_AddHandCuboids
        numberOfCuboids+=XR_HAND_JOINT_COUNT_EXT*2;
// XR_DOCS_TAG_BEGIN_AddHandCuboids
        m_uniformBuffer_Camera = m_graphicsAPI->CreateBuffer({GraphicsAPI::BufferCreateInfo::Type::UNIFORM, 0, sizeof(CameraConstants) * numberOfCuboids, nullptr});
        m_uniformBuffer_Normals = m_graphicsAPI->CreateBuffer({GraphicsAPI::BufferCreateInfo::Type::UNIFORM, 0, sizeof(normals), &normals});

        // XR_DOCS_TAG_END_CreateResources1
        // XR_DOCS_TAG_BEGIN_CreateResources2_OpenGL_Vulkan
        if (m_apiType == OPENGL) {
            std::string vertexSource = ReadTextFile("VertexShader.glsl");
            m_vertexShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::VERTEX, vertexSource.data(), vertexSource.size()});

            std::string fragmentSource = ReadTextFile("PixelShader.glsl");
            m_fragmentShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::FRAGMENT, fragmentSource.data(), fragmentSource.size()});
        }
        if (m_apiType == VULKAN) {
#if defined(__ANDROID__)
            std::vector<char> vertexSource = ReadBinaryFile("shaders/VertexShader.spv", androidApp->activity->assetManager);
#else
            std::vector<char> vertexSource = ReadBinaryFile("VertexShader.spv");
#endif
            m_vertexShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::VERTEX, vertexSource.data(), vertexSource.size()});

#if defined(__ANDROID__)
            std::vector<char> fragmentSource = ReadBinaryFile("shaders/PixelShader.spv", androidApp->activity->assetManager);
#else
            std::vector<char> fragmentSource = ReadBinaryFile("PixelShader.spv");
#endif
            m_fragmentShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::FRAGMENT, fragmentSource.data(), fragmentSource.size()});
        }
        // XR_DOCS_TAG_END_CreateResources2_OpenGL_Vulkan
        // XR_DOCS_TAG_BEGIN_CreateResources2_OpenGLES
        if (m_apiType == OPENGL_ES) {
#if defined(__ANDROID__)
            std::string vertexSource = ReadTextFile("shaders/VertexShader_GLES.glsl", androidApp->activity->assetManager);
#else
            std::string vertexSource = ReadTextFile("VertexShader_GLES.glsl");
#endif
            m_vertexShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::VERTEX, vertexSource.data(), vertexSource.size()});

#if defined(__ANDROID__)
            std::string fragmentSource = ReadTextFile("shaders/PixelShader_GLES.glsl", androidApp->activity->assetManager);
#else
            std::string fragmentSource = ReadTextFile("PixelShader_GLES.glsl");
#endif
            m_fragmentShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::FRAGMENT, fragmentSource.data(), fragmentSource.size()});
        }
        // XR_DOCS_TAG_END_CreateResources2_OpenGLES
        // XR_DOCS_TAG_BEGIN_CreateResources2_D3D
        if (m_apiType == D3D11) {
            std::vector<char> vertexSource = ReadBinaryFile("VertexShader_5_0.cso");
            m_vertexShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::VERTEX, vertexSource.data(), vertexSource.size()});

            std::vector<char> fragmentSource = ReadBinaryFile("PixelShader_5_0.cso");
            m_fragmentShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::FRAGMENT, fragmentSource.data(), fragmentSource.size()});
        }
        if (m_apiType == D3D12) {
            std::vector<char> vertexSource = ReadBinaryFile("VertexShader_5_1.cso");
            m_vertexShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::VERTEX, vertexSource.data(), vertexSource.size()});

            std::vector<char> fragmentSource = ReadBinaryFile("PixelShader_5_1.cso");
            m_fragmentShader = m_graphicsAPI->CreateShader({GraphicsAPI::ShaderCreateInfo::Type::FRAGMENT, fragmentSource.data(), fragmentSource.size()});
        }
        // XR_DOCS_TAG_END_CreateResources2_D3D
        // XR_DOCS_TAG_BEGIN_CreateResources3

        GraphicsAPI::PipelineCreateInfo pipelineCI;
        pipelineCI.shaders = {m_vertexShader, m_fragmentShader};
        pipelineCI.vertexInputState.attributes = {{0, 0, GraphicsAPI::VertexType::VEC4, 0, "TEXCOORD"}};
        pipelineCI.vertexInputState.bindings = {{0, 0, 4 * sizeof(float)}};
        pipelineCI.inputAssemblyState = {GraphicsAPI::PrimitiveTopology::TRIANGLE_LIST, false};
        pipelineCI.rasterisationState = {false, false, GraphicsAPI::PolygonMode::FILL, GraphicsAPI::CullMode::BACK, GraphicsAPI::FrontFace::COUNTER_CLOCKWISE, false, 0.0f, 0.0f, 0.0f, 1.0f};
        pipelineCI.multisampleState = {1, false, 1.0f, 0xFFFFFFFF, false, false};
        pipelineCI.depthStencilState = {true, true, GraphicsAPI::CompareOp::LESS_OR_EQUAL, false, false, {}, {}, 0.0f, 1.0f};
        pipelineCI.colourBlendState = {false, GraphicsAPI::LogicOp::NO_OP, {{true, GraphicsAPI::BlendFactor::SRC_ALPHA, GraphicsAPI::BlendFactor::ONE_MINUS_SRC_ALPHA, GraphicsAPI::BlendOp::ADD, GraphicsAPI::BlendFactor::ONE, GraphicsAPI::BlendFactor::ZERO, GraphicsAPI::BlendOp::ADD, (GraphicsAPI::ColourComponentBit)15}}, {0.0f, 0.0f, 0.0f, 0.0f}};
        pipelineCI.colorFormats = {m_swapchainAndDepthImages[0].swapchainFormat};
        pipelineCI.depthFormat = m_graphicsAPI->GetDepthFormat();
        pipelineCI.layout = {{0, nullptr, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::VERTEX},
                             {1, nullptr, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::VERTEX},
                             {2, nullptr, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::FRAGMENT}};
        m_pipeline = m_graphicsAPI->CreatePipeline(pipelineCI);
        
        float scale=0.2f;
        XrVector3f centre = {0,-0.2f, -0.7f};
        for (int i = 0; i < 4; i++) {
            float x = scale * (float(i) - 1.5f) + centre.x;
            for (int j = 0; j < 4; j++) {
                float y = scale * (float(j) - 1.5f) + centre.y;
                for (int k = 0; k < 4; k++) {
                    float angleRad = 0;
                    float z = scale * (float(k) - 1.5f) + centre.z;
                    XrQuaternionf q;
                    XrVector3f axis = {0, 0.707f, 0.707f};
                    XrQuaternionf_CreateFromAxisAngle(&q, &axis, angleRad);
                    XrVector3f colour = {pseudorandom_distribution(pseudo_random_generator), pseudorandom_distribution(pseudo_random_generator), pseudorandom_distribution(pseudo_random_generator)};
                    blocks.push_back({{q, {x, y, z}}, {0.095f, 0.095f, 0.095f},colour});
                }
            }
        }
    }
    // XR_DOCS_TAG_END_CreateResources3
    // XR_DOCS_TAG_BEGIN_DestroyResources
    void DestroyResources() {
        m_graphicsAPI->DestroyPipeline(m_pipeline);
        m_graphicsAPI->DestroyShader(m_fragmentShader);
        m_graphicsAPI->DestroyShader(m_vertexShader);
        m_graphicsAPI->DestroyBuffer(m_uniformBuffer_Camera);
        m_graphicsAPI->DestroyBuffer(m_uniformBuffer_Normals);
        m_graphicsAPI->DestroyBuffer(m_indexBuffer);
        m_graphicsAPI->DestroyBuffer(m_vertexBuffer);
    }
    // XR_DOCS_TAG_END_DestroyResources

// XR_DOCS_TAG_BEGIN_PollEvents
    void PollEvents() {
        XrResult result = XR_SUCCESS;
        do {
            XrEventDataBuffer eventData{XR_TYPE_EVENT_DATA_BUFFER};
            result = xrPollEvent(m_xrInstance, &eventData);

            switch (eventData.type) {
            case XR_TYPE_EVENT_DATA_EVENTS_LOST: {
                XrEventDataEventsLost *eventsLost = reinterpret_cast<XrEventDataEventsLost *>(&eventData);
                std::cout << "OPENXR: Events Lost: " << eventsLost->lostEventCount << std::endl;
                break;
            }
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: {
                XrEventDataInstanceLossPending *instanceLossPending = reinterpret_cast<XrEventDataInstanceLossPending *>(&eventData);
                std::cout << "OPENXR: Instance Loss Pending at: " << instanceLossPending->lossTime << std::endl;
                m_sessionRunning = false;
                m_applicationRunning = false;
                break;
            }
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED: {
                XrEventDataInteractionProfileChanged *interactionProfileChanged = reinterpret_cast<XrEventDataInteractionProfileChanged *>(&eventData);
                std::cout << "OPENXR: Interaction Profile changed for Session: " << interactionProfileChanged->session << std::endl;                
// XR_DOCS_TAG_BEGIN_CallRecordCurrentBindings
                RecordCurrentBindings();
// XR_DOCS_TAG_END_CallRecordCurrentBindings
                break;
            }
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING: {
                XrEventDataReferenceSpaceChangePending *referenceSpaceChangePending = reinterpret_cast<XrEventDataReferenceSpaceChangePending *>(&eventData);
                std::cout << "OPENXR: Reference Space Change pending for Session: " << referenceSpaceChangePending->session << std::endl;
                break;
            }
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED: {
                XrEventDataSessionStateChanged *sessionStateChanged = reinterpret_cast<XrEventDataSessionStateChanged *>(&eventData);

                if (sessionStateChanged->state == XR_SESSION_STATE_READY) {
                    XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
                    sessionBeginInfo.primaryViewConfigurationType = m_viewConfiguration;
                    OPENXR_CHECK(xrBeginSession(m_session, &sessionBeginInfo), "Failed to begin Session.");
                    m_sessionRunning = true;
                }
                if (sessionStateChanged->state == XR_SESSION_STATE_STOPPING) {
                    OPENXR_CHECK(xrEndSession(m_session), "Failed to end Session.");
                    m_sessionRunning = false;
                }
                if (sessionStateChanged->state == XR_SESSION_STATE_EXITING) {
                    m_sessionRunning = false;
                    m_applicationRunning = false;
                }
                m_sessionState = sessionStateChanged->state;
                break;
            }
            default: {
                break;
            }
            }

        } while (result == XR_SUCCESS);
    }
// XR_DOCS_TAG_END_PollEvents
    // XR_DOCS_TAG_BEGIN_PollActions
    void PollActions(XrTime predictedTime) {
        // Update our action set with up-to-date input data!
        XrActiveActionSet activeActionSet{};
        activeActionSet.actionSet = m_actionSet;
        activeActionSet.subactionPath = XR_NULL_PATH;

        XrActionsSyncInfo actionsSyncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        actionsSyncInfo.countActiveActionSets = 1;
        actionsSyncInfo.activeActionSets = &activeActionSet;
        OPENXR_CHECK(xrSyncActions(m_session, &actionsSyncInfo), "Failed to sync Actions.");
// XR_DOCS_TAG_END_PollActions
// XR_DOCS_TAG_BEGIN_PollActions2
        XrActionStateGetInfo actionStateGetInfo{XR_TYPE_ACTION_STATE_GET_INFO};
        actionStateGetInfo.action = m_controllerGripPoseAction;
        for(int i=0;i<2;i++) {
            actionStateGetInfo.subactionPath=m_handPaths[i];
            OPENXR_CHECK(xrGetActionStatePose(m_session, &actionStateGetInfo, &m_controllerGripPoseState[i]), "Failed to get Pose State.");
            if (m_controllerGripPoseState[i].isActive) {
                XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
                XrResult res = xrLocateSpace(m_controllerGripPoseSpace[i], m_localOrStageSpace, predictedTime, &spaceLocation);
                if (XR_UNQUALIFIED_SUCCESS(res) &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                    m_controllerGripPose[i] = spaceLocation.pose;
                }
            }
        }
// XR_DOCS_TAG_END_PollActions2
// XR_DOCS_TAG_BEGIN_PollActions3
        for(int i=0;i<2;i++) {
            actionStateGetInfo.action = m_grabAction;
            actionStateGetInfo.subactionPath=m_handPaths[i];
            OPENXR_CHECK(xrGetActionStateFloat(m_session, &actionStateGetInfo, &m_grabState[i]), "Failed to get Float State.");
            buzz[i]*=0.5f;
            if(buzz[i]<0.01f)
                buzz[i]=0.0f;
            XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
            vibration.amplitude = buzz[i];
            vibration.duration = XR_MIN_HAPTIC_DURATION;
            vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
        
            XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
            hapticActionInfo.action = m_buzzAction;
            hapticActionInfo.subactionPath = m_handPaths[i];
            OPENXR_CHECK(xrApplyHapticFeedback(m_session, &hapticActionInfo, (XrHapticBaseHeader*)&vibration), "Failed to apply haptic feedback.");
        }
// XR_DOCS_TAG_END_PollActions3
// XR_DOCS_TAG_BEGIN_PollActions4
        actionStateGetInfo.action = m_controllerGripPoseAction;
        for (int i = 0; i < 2; i++) {
            actionStateGetInfo.subactionPath = m_handPaths[i];
            OPENXR_CHECK(xrGetActionStatePose(m_session, &actionStateGetInfo, &m_controllerGripPoseState[i]), "Failed to get Pose State.");
            if (m_controllerGripPoseState[i].isActive) {
                XrSpaceLocation spaceLocation{XR_TYPE_SPACE_LOCATION};
                XrResult res = xrLocateSpace(m_controllerGripPoseSpace[i], m_localOrStageSpace, predictedTime, &spaceLocation);
                if (XR_UNQUALIFIED_SUCCESS(res) &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
                    (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0) {
                    m_controllerGripPose[i] = spaceLocation.pose;
                }
            }
        }
// XR_DOCS_TAG_BEGIN_PollHands
		if(handTrackingSystemProperties.supportsHandTracking)
		{
			XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
			XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
			for(int i=0;i<2;i++)
			{
				bool Unobstructed=true;
				Hand &hand=m_hands[i];
				XrHandJointsMotionRangeInfoEXT motionRangeInfo{XR_TYPE_HAND_JOINTS_MOTION_RANGE_INFO_EXT};
				motionRangeInfo.handJointsMotionRange = Unobstructed
															? XR_HAND_JOINTS_MOTION_RANGE_UNOBSTRUCTED_EXT
															: XR_HAND_JOINTS_MOTION_RANGE_CONFORMING_TO_CONTROLLER_EXT;
				XrHandJointsLocateInfoEXT locateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT, &motionRangeInfo};
				locateInfo.baseSpace = m_localOrStageSpace;
				locateInfo.time =predictedTime;
        
				XrHandJointLocationsEXT locations{XR_TYPE_HAND_JOINT_LOCATIONS_EXT};
				locations.jointCount = (uint32_t)XR_HAND_JOINT_COUNT_EXT;
				locations.jointLocations = hand.m_jointLocations;
				OPENXR_CHECK(xrLocateHandJointsEXT(hand.m_handTracker, &locateInfo, &locations), "Failed to locate hand joints.");
			}
		}
        
// XR_DOCS_TAG_END_PollHands
    }
    
    // XR_DOCS_TAG_END_PollActions4
    // XR_DOCS_TAG_BEGIN_BlockInteraction
    static XrVector3f FixPosition(XrVector3f pos)
    {
        int x=std::nearbyint(pos.x*10.f);
        int y=std::nearbyint(pos.y*10.f);
        int z=std::nearbyint(pos.z*10.f);
        pos.x=float(x)/10.f;
        pos.y=float(y)/10.f;
        pos.z=float(z)/10.f;
        return pos;
    }
    void BlockInteraction() {
        for(int i=0;i<2;i++)
        {
            float nearest=1.0f;
            if(grabbedBlock[i]==-1) {
                nearBlock[i]=-1;
                if(m_controllerGripPoseState[i].isActive) {
                    for(int j=0;j<blocks.size();j++) {
                        auto block=blocks[j];
                        XrVector3f diff=block.pose.position-m_controllerGripPose[i].position;
                        float distance=std::max(fabs(diff.x),std::max(fabs(diff.y),fabs(diff.z)));
                        if(distance<0.1f&&distance<nearest) {
                            nearBlock[i]=j;
                            nearest=distance;
                        }
                    }
                }
                if(nearBlock[i]!=-1&&m_grabState[i].isActive&&m_grabState[i].currentState>0.5f) {
                    grabbedBlock[i]=nearBlock[i];
                    buzz[i]=1.0f;
                }
            }
            else
            {
                nearBlock[i]=grabbedBlock[i];
                if(m_controllerGripPoseState[i].isActive)
                    blocks[grabbedBlock[i]].pose.position=m_controllerGripPose[i].position;
                if(!m_grabState[i].isActive||m_grabState[i].currentState<0.5f) {
                    blocks[grabbedBlock[i]].pose.position=FixPosition(blocks[grabbedBlock[i]].pose.position);
                    grabbedBlock[i]=-1;
                    buzz[i]=0.2f;
                }
            }

        }
    }
    // XR_DOCS_TAG_END_BlockInteraction

    // XR_DOCS_TAG_BEGIN_CreateReferenceSpace
    void CreateReferenceSpace() {
        XrReferenceSpaceCreateInfo referenceSpaceCI{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        referenceSpaceCI.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        referenceSpaceCI.poseInReferenceSpace = {{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 0.0f}};
        OPENXR_CHECK(xrCreateReferenceSpace(m_session, &referenceSpaceCI, &m_localOrStageSpace), "Failed to create ReferenceSpace.");
    }
    // XR_DOCS_TAG_END_CreateReferenceSpace

    // XR_DOCS_TAG_BEGIN_DestroyReferenceSpace
    void DestroyReferenceSpace() {
        OPENXR_CHECK(xrDestroySpace(m_localOrStageSpace), "Failed to destroy Space.")
    }
    // XR_DOCS_TAG_END_DestroyReferenceSpace

    void CreateSwapchain() {
        // XR_DOCS_TAG_BEGIN_EnumerateSwapchainFormats
        uint32_t formatSize = 0;
        OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, 0, &formatSize, nullptr), "Failed to enumerate Swapchain Formats");
        std::vector<int64_t> formats(formatSize);
        OPENXR_CHECK(xrEnumerateSwapchainFormats(m_session, formatSize, &formatSize, formats.data()), "Failed to enumerate Swapchain Formats");
        // XR_DOCS_TAG_END_EnumerateSwapchainFormats

        // Check the two views for stereo are the same
        if (m_viewConfiguration == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO && m_viewConfigurationViews.size() == 2) {
            bool viewWidthsSame = m_viewConfigurationViews[0].recommendedImageRectWidth == m_viewConfigurationViews[1].recommendedImageRectWidth;
            bool viewHeightsSame = m_viewConfigurationViews[0].recommendedImageRectHeight == m_viewConfigurationViews[1].recommendedImageRectHeight;
            if (!viewWidthsSame || !viewHeightsSame) {
                std::cout << "ERROR: The two views for stereo are not the same." << std::endl;
                DEBUG_BREAK;
            }
        }
        const XrViewConfigurationView &viewConfigurationView = m_viewConfigurationViews[0];

        m_swapchainAndDepthImages.resize(m_viewConfigurationViews.size());
        for (SwapchainAndDepthImage &swapchainAndDepthImage : m_swapchainAndDepthImages) {
            // XR_DOCS_TAG_BEGIN_CreateSwapchain
            XrSwapchainCreateInfo swapchainCI{XR_TYPE_SWAPCHAIN_CREATE_INFO};
            swapchainCI.createFlags = 0;
            swapchainCI.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
            swapchainCI.format = m_graphicsAPI->SelectSwapchainFormat(formats);
            swapchainCI.sampleCount = viewConfigurationView.recommendedSwapchainSampleCount;
            swapchainCI.width = viewConfigurationView.recommendedImageRectWidth;
            swapchainCI.height = viewConfigurationView.recommendedImageRectHeight;
            swapchainCI.faceCount = 1;
            swapchainCI.arraySize = 1;
            swapchainCI.mipCount = 1;
            OPENXR_CHECK(xrCreateSwapchain(m_session, &swapchainCI, &swapchainAndDepthImage.swapchain), "Failed to create Swapchain");
            swapchainAndDepthImage.swapchainFormat = swapchainCI.format;
            // XR_DOCS_TAG_END_CreateSwapchain

            // XR_DOCS_TAG_BEGIN_EnumerateSwapchainImages
            uint32_t swapchainImageCount = 0;
            OPENXR_CHECK(xrEnumerateSwapchainImages(swapchainAndDepthImage.swapchain, 0, &swapchainImageCount, nullptr), "Failed to enumerate Swapchain Images.");
            XrSwapchainImageBaseHeader *swapchainImages = m_graphicsAPI->AllocateSwapchainImageData(swapchainImageCount);
            OPENXR_CHECK(xrEnumerateSwapchainImages(swapchainAndDepthImage.swapchain, swapchainImageCount, &swapchainImageCount, swapchainImages), "Failed to enumerate Swapchain Images.");
            // XR_DOCS_TAG_END_EnumerateSwapchainImages

            // XR_DOCS_TAG_BEGIN_CreateDepthImage
            GraphicsAPI::ImageCreateInfo depthImageCI;
            depthImageCI.dimension = 2;
            depthImageCI.width = viewConfigurationView.recommendedImageRectWidth;
            depthImageCI.height = viewConfigurationView.recommendedImageRectHeight;
            depthImageCI.depth = 1;
            depthImageCI.mipLevels = 1;
            depthImageCI.arrayLayers = 1;
            depthImageCI.sampleCount = 1;
            depthImageCI.format = m_graphicsAPI->GetDepthFormat();
            depthImageCI.cubemap = false;
            depthImageCI.colorAttachment = false;
            depthImageCI.depthAttachment = true;
            depthImageCI.sampled = false;
            swapchainAndDepthImage.depthImage = m_graphicsAPI->CreateImage(depthImageCI);
            // XR_DOCS_TAG_END_CreateDepthImage

            // XR_DOCS_TAG_BEGIN_CreateImageViews
            for (uint32_t i = 0; i < swapchainImageCount; i++) {
                GraphicsAPI::ImageViewCreateInfo imageViewCI;
                imageViewCI.image = m_graphicsAPI->GetSwapchainImage(i);
                imageViewCI.type = GraphicsAPI::ImageViewCreateInfo::Type::RTV;
                imageViewCI.view = GraphicsAPI::ImageViewCreateInfo::View::TYPE_2D;
                imageViewCI.format = swapchainAndDepthImage.swapchainFormat;
                imageViewCI.aspect = GraphicsAPI::ImageViewCreateInfo::Aspect::COLOR_BIT;
                imageViewCI.baseMipLevel = 0;
                imageViewCI.levelCount = 1;
                imageViewCI.baseArrayLayer = 0;
                imageViewCI.layerCount = 1;
                swapchainAndDepthImage.colorImageViews.push_back(m_graphicsAPI->CreateImageView(imageViewCI));
            }

            GraphicsAPI::ImageViewCreateInfo imageViewCI;
            imageViewCI.image = swapchainAndDepthImage.depthImage;
            imageViewCI.type = GraphicsAPI::ImageViewCreateInfo::Type::DSV;
            imageViewCI.view = GraphicsAPI::ImageViewCreateInfo::View::TYPE_2D;
            imageViewCI.format = m_graphicsAPI->GetDepthFormat();
            imageViewCI.aspect = GraphicsAPI::ImageViewCreateInfo::Aspect::DEPTH_BIT;
            imageViewCI.baseMipLevel = 0;
            imageViewCI.levelCount = 1;
            imageViewCI.baseArrayLayer = 0;
            imageViewCI.layerCount = 1;
            swapchainAndDepthImage.depthImageView = m_graphicsAPI->CreateImageView(imageViewCI);
            // XR_DOCS_TAG_END_CreateImageViews
        }
    }


    // XR_DOCS_TAG_BEGIN_DestroySwapchain
    void DestroySwapchain() {
        for (SwapchainAndDepthImage &swapchainAndDepthImage : m_swapchainAndDepthImages) {
            m_graphicsAPI->DestroyImageView(swapchainAndDepthImage.depthImageView);
            for (void *&colorImageView : swapchainAndDepthImage.colorImageViews) {
                m_graphicsAPI->DestroyImageView(colorImageView);
            }

            m_graphicsAPI->DestroyImage(swapchainAndDepthImage.depthImage);

            OPENXR_CHECK(xrDestroySwapchain(swapchainAndDepthImage.swapchain), "Failed to destroy Swapchain");
        }
    }

    // XR_DOCS_TAG_END_DestroySwapchain

    // XR_DOCS_TAG_BEGIN_RenderCuboid
    size_t renderCuboidIndex = 0;
    void RenderCuboid(XrPosef pose, XrVector3f scale,XrVector3f colour) {
        XrMatrix4x4f_CreateTranslationRotationScale(&cameraConstants.model, &pose.position, &pose.orientation, &scale);

        XrMatrix4x4f_Multiply(&cameraConstants.modelViewProj, &cameraConstants.viewProj, &cameraConstants.model);
        cameraConstants.colour={colour.x,colour.y,colour.z,1.0};
        size_t offsetCameraUB = sizeof(CameraConstants) * renderCuboidIndex;

        m_graphicsAPI->SetPipeline(m_pipeline);

        m_graphicsAPI->SetBufferData(m_uniformBuffer_Camera, offsetCameraUB, sizeof(CameraConstants), &cameraConstants);
        m_graphicsAPI->SetDescriptor({0, m_uniformBuffer_Camera, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::VERTEX, false, offsetCameraUB, sizeof(CameraConstants)});
        m_graphicsAPI->SetDescriptor({1, m_uniformBuffer_Normals, GraphicsAPI::DescriptorInfo::Type::BUFFER, GraphicsAPI::DescriptorInfo::Stage::VERTEX, false, 0, sizeof(normals)});

        m_graphicsAPI->UpdateDescriptors();

        m_graphicsAPI->SetVertexBuffers(&m_vertexBuffer, 1);
        m_graphicsAPI->SetIndexBuffer(m_indexBuffer);
        m_graphicsAPI->DrawIndexed(36);

        renderCuboidIndex++;
    }
    // XR_DOCS_TAG_END_RenderCuboid
    // XR_DOCS_TAG_BEGIN_RenderFrame
    void RenderFrame() {
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_3_2
        XrFrameState frameState{XR_TYPE_FRAME_STATE};
        XrFrameWaitInfo frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
        OPENXR_CHECK(xrWaitFrame(m_session, &frameWaitInfo, &frameState), "Failed to wait for XR Frame.");

        XrFrameBeginInfo frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        OPENXR_CHECK(xrBeginFrame(m_session, &frameBeginInfo), "Failed to begin the XR Frame.");

        bool rendered = false;
        std::vector<XrCompositionLayerBaseHeader *> layers;
        XrCompositionLayerProjection layerProjection{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
        std::vector<XrCompositionLayerProjectionView> layerProjectionViews;

        bool sessionActive = (m_sessionState == XR_SESSION_STATE_SYNCHRONIZED || m_sessionState == XR_SESSION_STATE_VISIBLE || m_sessionState == XR_SESSION_STATE_FOCUSED);
        if (sessionActive && frameState.shouldRender) {
#if XR_DOCS_CHAPTER_VERSION >= XR_DOCS_CHAPTER_4_2
// XR_DOCS_TAG_BEGIN_CallPollActions
            // poll actions here because they require a predicted display time
            PollActions(frameState.predictedDisplayTime);
            BlockInteraction();
// XR_DOCS_TAG_END_CallPollActions
#endif
            rendered = RenderLayer(frameState.predictedDisplayTime, layerProjection, layerProjectionViews);
            if (rendered) {
                layers.push_back(reinterpret_cast<XrCompositionLayerBaseHeader *>(&layerProjection));
            }
        }

        XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
        frameEndInfo.displayTime = frameState.predictedDisplayTime;
        frameEndInfo.environmentBlendMode = m_environmentBlendMode;
        frameEndInfo.layerCount = static_cast<uint32_t>(layers.size());
        frameEndInfo.layers = layers.data();
        OPENXR_CHECK(xrEndFrame(m_session, &frameEndInfo), "Failed to end the XR Frame.");
#endif
    }
    // XR_DOCS_TAG_END_RenderFrame


    // XR_DOCS_TAG_BEGIN_RenderLayer
    bool RenderLayer(const XrTime &predictedDisplayTime, XrCompositionLayerProjection &layerProjection, std::vector<XrCompositionLayerProjectionView> &layerProjectionViews) {
        std::vector<XrView> views(m_viewConfigurationViews.size(), {XR_TYPE_VIEW});

        XrViewState viewState{XR_TYPE_VIEW_STATE};
        XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
        viewLocateInfo.viewConfigurationType = m_viewConfiguration;
        viewLocateInfo.displayTime = predictedDisplayTime;
        viewLocateInfo.space = m_localOrStageSpace;
        uint32_t viewCount = 0;
        XrResult result = xrLocateViews(m_session, &viewLocateInfo, &viewState, static_cast<uint32_t>(views.size()), &viewCount, views.data());
        if (result != XR_SUCCESS) {
            std::cout << "Failed to locate Views." << std::endl;
            return false;
        }

        layerProjectionViews.resize(viewCount, {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW});
        for (uint32_t i = 0; i < viewCount; i++) {
            uint32_t imageIndex = 0;
            XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
            OPENXR_CHECK(xrAcquireSwapchainImage(m_swapchainAndDepthImages[i].swapchain, &acquireInfo, &imageIndex), "Failed to acquire Image from the Swapchian");

            XrSwapchainImageWaitInfo waitInfo = {XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
            waitInfo.timeout = XR_INFINITE_DURATION;
            OPENXR_CHECK(xrWaitSwapchainImage(m_swapchainAndDepthImages[i].swapchain, &waitInfo), "Failed to wait for Image from the Swapchain");

            const uint32_t &width = m_viewConfigurationViews[i].recommendedImageRectWidth;
            const uint32_t &height = m_viewConfigurationViews[i].recommendedImageRectHeight;
            GraphicsAPI::Viewport viewport = {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
            GraphicsAPI::Rect2D scissor = {{(int32_t)0, (int32_t)0}, {width, height}};

            layerProjectionViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
            layerProjectionViews[i].pose = views[i].pose;
            layerProjectionViews[i].fov = views[i].fov;
            layerProjectionViews[i].subImage.swapchain = m_swapchainAndDepthImages[i].swapchain;
            layerProjectionViews[i].subImage.imageRect.offset.x = 0;
            layerProjectionViews[i].subImage.imageRect.offset.y = 0;
            layerProjectionViews[i].subImage.imageRect.extent.width = static_cast<int32_t>(width);
            layerProjectionViews[i].subImage.imageRect.extent.height = static_cast<int32_t>(height);
            layerProjectionViews[i].subImage.imageArrayIndex = 0;

            m_graphicsAPI->BeginRendering();
            if (m_environmentBlendMode == XR_ENVIRONMENT_BLEND_MODE_OPAQUE) {
                // VR mode use a background color.
                m_graphicsAPI->ClearColor(m_swapchainAndDepthImages[i].colorImageViews[imageIndex], 0.17f, 0.17f, 0.17f, 1.00f);
            } else {
                // In AR mode make the background color black.
                m_graphicsAPI->ClearColor(m_swapchainAndDepthImages[i].colorImageViews[imageIndex], 0.00f, 0.00f, 0.00f, 1.00f);
            }
            m_graphicsAPI->ClearDepth(m_swapchainAndDepthImages[i].depthImageView, 1.0f);

            // XR_DOCS_TAG_BEGIN_SetupFrameRendering
            m_graphicsAPI->SetRenderAttachments(&m_swapchainAndDepthImages[i].colorImageViews[imageIndex], 1, m_swapchainAndDepthImages[i].depthImageView, width, height, m_pipeline);
            m_graphicsAPI->SetViewports(&viewport, 1);
            m_graphicsAPI->SetScissors(&scissor, 1);

            // Compute the view-projection transform.
            // All matrices (including OpenXR's) are column-major, right-handed.
            XrMatrix4x4f proj;
            XrMatrix4x4f_CreateProjectionFov(&proj, m_apiType, views[i].fov, 0.05f, 100.0f);
            XrMatrix4x4f toView;
            XrVector3f scale1m{1.0f, 1.0f, 1.0f};
            XrMatrix4x4f_CreateTranslationRotationScale(&toView, &views[i].pose.position, &views[i].pose.orientation, &scale1m);
            XrMatrix4x4f view;
            XrMatrix4x4f_InvertRigidBody(&view, &toView);
            XrMatrix4x4f_Multiply(&cameraConstants.viewProj, &proj, &view);

            // XR_DOCS_TAG_END_SetupFrameRendering
            // XR_DOCS_TAG_BEGIN_CallRenderCuboid
            renderCuboidIndex = 0;
            // Draw a floor. Scale it by 2 in the X and Z, and 0.1 in the Y,
            RenderCuboid({{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, -m_viewHeightM, 0.0f}}, {2.0f, 0.1f, 2.0f},{0.4f,0.5f,0.5f});
            // Draw a "table".
            RenderCuboid({{0.0f, 0.0f, 0.0f, 1.0f}, {0.0f, -m_viewHeightM+0.9f, -0.7f}}, {1.0f, 0.2f, 1.0f},{0.6f,0.6f,0.6f});

            // Draw some blocks at the controller positions:
           for (int i = 0; i < 2; i++) {
                if (m_controllerGripPoseState[i].isActive) {
                    RenderCuboid(m_controllerGripPose[i], {0.02f, 0.04f, 0.10f},{1.f,1.f,1.f});
                }
            }
            for(int i=0;i<blocks.size();i++) {
                auto p=blocks[i];
                XrVector3f sc=p.scale;
                if(i==nearBlock[0]||i==nearBlock[1])
                    sc=p.scale*1.05f;
                RenderCuboid(p.pose,sc,p.colour);
            }

            // XR_DOCS_TAG_END_CallRenderCuboid
			
            // XR_DOCS_TAG_BEGIN_RenderHands
			if(handTrackingSystemProperties.supportsHandTracking)
				for(int i=0;i<2;i++)
				{
					auto hand=m_hands[i];
					XrVector3f hand_colour={1.f,1.f,0.f};
					for(int j=0;j<XR_HAND_JOINT_COUNT_EXT;j++)
					{
						XrVector3f sc={1.5f,1.5f,2.5f};
						sc=sc*hand.m_jointLocations[j].radius;
						RenderCuboid(hand.m_jointLocations[j].pose,sc,hand_colour);
					}
				}
            // XR_DOCS_TAG_END_RenderHands
            m_graphicsAPI->EndRendering();

            XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
            OPENXR_CHECK(xrReleaseSwapchainImage(m_swapchainAndDepthImages[i].swapchain, &releaseInfo), "Failed to release Image back to the Swapchain");
        };
        layerProjection.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT | XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
        layerProjection.space = m_localOrStageSpace;
        layerProjection.viewCount = static_cast<uint32_t>(layerProjectionViews.size());
        layerProjection.views = layerProjectionViews.data();

        return true;
    }

#if defined(__ANDROID__)
    // XR_DOCS_TAG_BEGIN_Android_System_Functionality
public:
    static android_app *androidApp;

    // Modified from https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/d6b6d7a10bdcf8d4fe806b4f415fde3dd5726878/src/tests/hello_xr/main.cpp#L133C1-L189C2
    struct AndroidAppState {
        ANativeWindow *nativeWindow = nullptr;
        bool resumed = false;
    };
    static AndroidAppState androidAppState;

    // Process the next main command.
    static void AndroidAppHandleCmd(struct android_app *app, int32_t cmd) {
        AndroidAppState *appState = (AndroidAppState *)app->userData;

        switch (cmd) {
        // There is no APP_CMD_CREATE. The ANativeActivity creates the application thread from onCreate().
        // The application thread then calls android_main().
        case APP_CMD_START: {
            break;
        }
        case APP_CMD_RESUME: {
            appState->resumed = true;
            break;
        }
        case APP_CMD_PAUSE: {
            appState->resumed = false;
            break;
        }
        case APP_CMD_STOP: {
            break;
        }
        case APP_CMD_DESTROY: {
            appState->nativeWindow = nullptr;
            break;
        }
        case APP_CMD_INIT_WINDOW: {
            appState->nativeWindow = app->window;
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            appState->nativeWindow = nullptr;
            break;
        }
        }
    }

private:
    void PollSystemEvents() {
        if (androidApp->destroyRequested != 0) {
            m_applicationRunning = false;
            return;
        }
        while (true) {
            struct android_poll_source *source = nullptr;
            int events = 0;
            const int timeoutMilliseconds = (!androidAppState.resumed && !m_sessionRunning && androidApp->destroyRequested == 0) ? -1 : 0;
            if (ALooper_pollAll(timeoutMilliseconds, nullptr, &events, (void **)&source) >= 0) {
                if (source != nullptr) {
                    source->process(androidApp, source);
                }
            } else {
                break;
            }
        }
    }
// XR_DOCS_TAG_END_Android_System_Functionality
#else
    void PollSystemEvents() {
        return;
    }
#endif

private:
    XrInstance m_xrInstance = {};
    XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
    std::vector<const char *> m_activeAPILayers = {};
    std::vector<const char *> m_activeInstanceExtensions = {};
    std::vector<std::string> m_apiLayers = {};
    std::vector<std::string> m_instanceExtensions = {};

    XrDebugUtilsMessengerEXT m_debugUtilsMessenger = {};

    XrFormFactor m_formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId m_systemID = {};

    GraphicsAPI_Type m_apiType = UNKNOWN;
    std::unique_ptr<GraphicsAPI> m_graphicsAPI = nullptr;

    XrViewConfigurationType m_viewConfiguration = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    std::vector<XrViewConfigurationView> m_viewConfigurationViews;

    XrSession m_session = {};
    XrSessionState m_sessionState = XR_SESSION_STATE_UNKNOWN;
    bool m_applicationRunning = true;
    bool m_sessionRunning = false;

    struct SwapchainAndDepthImage {
        XrSwapchain swapchain = {};
        int64_t swapchainFormat = 0;
        void *depthImage = nullptr;

        std::vector<void *> colorImageViews;
        void *depthImageView = nullptr;
    };
    std::vector<SwapchainAndDepthImage> m_swapchainAndDepthImages = {};

    std::vector<XrEnvironmentBlendMode> m_applicationEnvironmentBlendModes = {XR_ENVIRONMENT_BLEND_MODE_OPAQUE, XR_ENVIRONMENT_BLEND_MODE_ADDITIVE};
    std::vector<XrEnvironmentBlendMode> m_environmentBlendModes = {};
    XrEnvironmentBlendMode m_environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_MAX_ENUM;

    XrSpace m_localOrStageSpace = {};
    // XR_DOCS_TAG_BEGIN_DeclareResources
    // In STAGE space, viewHeightM should be 0. In LOCAL space, it should be offset downwards, below the viewer's initial position.
    float m_viewHeightM = 1.5f;

    void *m_vertexBuffer = nullptr;
    void *m_indexBuffer = nullptr;
    void *m_uniformBuffer_Camera = nullptr;
    void *m_uniformBuffer_Normals = nullptr;

    void *m_vertexShader = nullptr, *m_fragmentShader = nullptr;
    void *m_pipeline = nullptr;
    // XR_DOCS_TAG_END_DeclareResources

    // XR_DOCS_TAG_BEGIN_Actions
    XrActionSet m_actionSet;
    // An action for grabbing blocks.
    XrAction m_grabAction;
    XrActionStateFloat m_grabState[2] = {{XR_TYPE_ACTION_STATE_FLOAT},{XR_TYPE_ACTION_STATE_FLOAT}};
    // The action haptic vibration of the right controller.
    XrAction m_buzzAction;
    float buzz[2]={0,0};
    // The action for getting the left grip pose.
    XrAction m_controllerGripPoseAction;
    // The XrPaths for left and right hand controllers.
    XrPath m_handPaths[2] = {0, 0};
    // The space that represents the left grip pose.
    XrSpace m_controllerGripPoseSpace[2];
    XrActionStatePose m_controllerGripPoseState[2] = {{XR_TYPE_ACTION_STATE_POSE}, {XR_TYPE_ACTION_STATE_POSE}};
    // The current left grip pose obtained from the XrSpace.
    XrPosef m_controllerGripPose[2];
    // XR_DOCS_TAG_END_Actions

    
    // XR_DOCS_TAG_BEGIN_Objects
    struct Block
    {
        XrPosef pose;
        XrVector3f scale;
        XrVector3f colour;
    };
    std::vector<Block> blocks;
    int grabbedBlock[2]={-1,-1};
    int nearBlock[2]={-1,-1};
// XR_DOCS_TAG_END_Objects

// XR_DOCS_TAG_BEGIN_HandTracking
	XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
    struct Hand
    {
        XrHandJointLocationEXT m_jointLocations[XR_HAND_JOINT_COUNT_EXT]; 
        XrHandTrackerEXT  m_handTracker=0;
    };
    Hand m_hands[2];
// XR_DOCS_TAG_END_HandTracking
};

void OpenXRTutorial_Main(GraphicsAPI_Type api) {
    DebugOutput debugOutput;
    std::cout << "OpenXR Tutorial Chapter 4." << std::endl;

    OpenXRTutorial app(api);
    app.Run();
}

#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__))
// XR_DOCS_TAG_BEGIN_main_WIN32___linux__
int main(int argc, char **argv) {
    OpenXRTutorial_Main(D3D11);
}
// XR_DOCS_TAG_END_main_WIN32___linux__
#elif (__ANDROID__)
// XR_DOCS_TAG_BEGIN_android_main___ANDROID__
android_app *OpenXRTutorial::androidApp = nullptr;
OpenXRTutorial::AndroidAppState OpenXRTutorial::androidAppState = {};

void android_main(struct android_app *app) {
    // Allow interaction with JNI and the JVM on this thread.
    // https://developer.android.com/training/articles/perf-jni#threads
    JNIEnv *env;
    app->activity->vm->AttachCurrentThread(&env, nullptr);

    XrInstance m_xrInstance = XR_NULL_HANDLE;  // Dummy XrInstance variable for OPENXR_CHECK macro.
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    OPENXR_CHECK(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR", (PFN_xrVoidFunction *)&xrInitializeLoaderKHR), "Failed to get InstanceProcAddr.");
    if (!xrInitializeLoaderKHR) {
        return;
    }

    XrLoaderInitInfoAndroidKHR loaderInitializeInfoAndroid{XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR};
    loaderInitializeInfoAndroid.applicationVM = app->activity->vm;
    loaderInitializeInfoAndroid.applicationContext = app->activity->clazz;
    OPENXR_CHECK(xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR *)&loaderInitializeInfoAndroid), "Failed to initialise Loader for Android.");

    app->userData = &OpenXRTutorial::androidAppState;
    app->onAppCmd = OpenXRTutorial::AndroidAppHandleCmd;

    OpenXRTutorial::androidApp = app;
    OpenXRTutorial_Main(VULKAN);
}
// XR_DOCS_TAG_END_android_main___ANDROID__
#endif