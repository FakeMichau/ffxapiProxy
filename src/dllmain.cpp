#include "log.h"
#include <dxgi.h>
#include <d3d12.h>
#include <ffx_api/ffx_api.hpp>
#include <ffx_api/ffx_upscale.h>
#include <ffx_api/dx12/ffx_api_dx12.h>

struct PresetOverrides {
    float nativeAA = 1.0;
    float quality  = 1.5;
    float balanced = 1.7;
    float performance = 2.0;
    float ultra_performance = 3.0;
};

struct Config
{
    std::vector<const char *> versionNames;
    std::vector<uint64_t> versionIds;
    u_int upscalerIndex = 0; // assuming 1 == 2.3.2
    bool debugView = true;
    bool disableReactiveMask = false;
    bool disableTransparencyMask = false;
    PresetOverrides presetOverrides;
} config;

static HMODULE _amdDll = nullptr;
static PfnFfxCreateContext _createContext = nullptr;
static PfnFfxDestroyContext _destroyContext = nullptr;
static PfnFfxConfigure _configure = nullptr;
static PfnFfxQuery _query = nullptr;
static PfnFfxDispatch _dispatch = nullptr;

FFX_API_ENTRY ffxReturnCode_t ffxCreateContext(ffxContext *context, ffxCreateContextDescHeader *header, const ffxAllocationCallbacks *memCb)
{
    if (_createContext == nullptr)
        return FFX_API_RETURN_ERROR;

    log("ffxCreateContext");

    for (const auto *it = header->pNext; it; it = it->pNext)
    {
        switch (it->type)
        {
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE:
            log("ffxCreateContext header->type: FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE");
            break;
        case FFX_API_DESC_TYPE_OVERRIDE_VERSION:
            log("ffxCreateContext header->type: FFX_API_DESC_TYPE_OVERRIDE_VERSION");
            break;
        case FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12:
        {
            log("ffxCreateContext header->type: FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12");
            ffx::QueryDescGetVersions versionQuery{};
            versionQuery.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
            versionQuery.device = reinterpret_cast<const ffxCreateBackendDX12Desc *>(it)->device;
            uint64_t versionCount = 0;
            versionQuery.outputCount = &versionCount;
            _query(nullptr, &versionQuery.header);

            config.versionIds.resize(versionCount);
            config.versionNames.resize(versionCount);
            versionQuery.versionIds = config.versionIds.data();
            versionQuery.versionNames = config.versionNames.data();
            _query(nullptr, &versionQuery.header);
            break;
        }
        default:
            log("ffxCreateContext header->type: {}", it->type);
        }
    }

    ffxOverrideVersion override{};
    override.header.type = FFX_API_DESC_TYPE_OVERRIDE_VERSION;
    override.versionId = config.versionIds[config.upscalerIndex];
    log("FFX_API_DESC_TYPE_OVERRIDE_VERSION: {}", config.versionNames[config.upscalerIndex]);
    override.header.pNext = header->pNext;
    header->pNext = &override.header;

    auto result = _createContext(context, header, memCb);

    log("ffxCreateContext result: {}", result);

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxDestroyContext(ffxContext *context, const ffxAllocationCallbacks *memCb)
{
    log("ffxDestroyContext");

    auto result = _destroyContext(context, memCb);

    log("ffxDestroyContext result: {}", result);

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxConfigure(ffxContext *context, const ffxConfigureDescHeader *desc)
{
    log("ffxConfigure");

    auto result = _configure(context, desc);

    log("ffxConfigure result: {}", result);

    return result;
}

typedef enum FfxFsr3UpscalerQualityMode {
    FFX_FSR3UPSCALER_QUALITY_MODE_NATIVEAA          = 0,
    FFX_FSR3UPSCALER_QUALITY_MODE_QUALITY           = 1,
    FFX_FSR3UPSCALER_QUALITY_MODE_BALANCED          = 2,
    FFX_FSR3UPSCALER_QUALITY_MODE_PERFORMANCE       = 3,
    FFX_FSR3UPSCALER_QUALITY_MODE_ULTRA_PERFORMANCE = 4 
} FfxFsr3UpscalerQualityMode;

float getRatioFromPreset(FfxFsr3UpscalerQualityMode qualityMode)
{
    switch (qualityMode) {
    case FFX_FSR3UPSCALER_QUALITY_MODE_NATIVEAA:
        return config.presetOverrides.nativeAA;
    case FFX_FSR3UPSCALER_QUALITY_MODE_QUALITY:
        return config.presetOverrides.quality;
    case FFX_FSR3UPSCALER_QUALITY_MODE_BALANCED:
        return config.presetOverrides.balanced;
    case FFX_FSR3UPSCALER_QUALITY_MODE_PERFORMANCE:
        return config.presetOverrides.performance;
    case FFX_FSR3UPSCALER_QUALITY_MODE_ULTRA_PERFORMANCE:
        return config.presetOverrides.ultra_performance;
    default:
        return 0.0f;
    }
}

FFX_API_ENTRY ffxReturnCode_t ffxQuery(ffxContext *context, ffxQueryDescHeader *header)
{
    log("ffxQuery");

    switch(header->type) {
    case FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE:
    {
        auto desc = reinterpret_cast<ffxQueryDescUpscaleGetRenderResolutionFromQualityMode*>(header);
        const float ratio = getRatioFromPreset((FfxFsr3UpscalerQualityMode)(desc->qualityMode));
        const uint32_t scaledDisplayWidth = (uint32_t)((float)desc->displayWidth / ratio);
        const uint32_t scaledDisplayHeight = (uint32_t)((float)desc->displayHeight / ratio);

        if (desc->pOutRenderWidth != nullptr)
            *desc->pOutRenderWidth = scaledDisplayWidth;
        if (desc->pOutRenderHeight != nullptr)
            *desc->pOutRenderHeight = scaledDisplayHeight;
        log("ffxQuery header->type: FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE");
        log("Output Resolution: {}x{}", *desc->pOutRenderWidth, *desc->pOutRenderHeight);
        break;
    }
    case FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE:
    {
        auto desc = reinterpret_cast<ffxQueryDescUpscaleGetUpscaleRatioFromQualityMode*>(header);

        if (desc->pOutUpscaleRatio != nullptr)
            *desc->pOutUpscaleRatio = static_cast<FfxFsr3UpscalerQualityMode>(desc->qualityMode);
        log("ffxQuery header->type: FFX_API_QUERY_DESC_TYPE_UPSCALE_GETUPSCALERATIOFROMQUALITYMODE");
        break;
    }
    }

    auto result = _query(context, header);

    log("ffxQuery result: {}", result);

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxDispatch(ffxContext *context, const ffxDispatchDescHeader *header)
{
    log("ffxDispatch");

    switch (header->type)
    {
    case FFX_API_DISPATCH_DESC_TYPE_UPSCALE:
    {
        auto ud = (ffxDispatchDescUpscale *)header;

        ud->flags |= config.debugView ? FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW : 0;

        if (config.disableReactiveMask)
            ud->reactive.resource = nullptr;

        if (config.disableTransparencyMask)
            ud->transparencyAndComposition.resource = nullptr;

        log("ffxDispatch desc->type: FFX_API_DISPATCH_DESC_TYPE_UPSCALE");
        log("ffxDispatch ud->cameraFar: {}", ud->cameraFar);
        log("ffxDispatch ud->cameraFovAngleVertical: {}", ud->cameraFovAngleVertical);
        log("ffxDispatch ud->cameraNear: {}", ud->cameraNear);
        log("ffxDispatch ud->color: {}null", ud->color.resource ? "not " : "");
        log("ffxDispatch ud->commandList: {}null", ud->commandList ? "not " : "");
        log("ffxDispatch ud->depth: {}null", ud->depth.resource ? "not " : "");
        log("ffxDispatch ud->enableSharpening: {}", ud->enableSharpening);
        log("ffxDispatch ud->exposure: {}null", ud->exposure.resource ? "not " : "");
        log("ffxDispatch ud->flags: {}", ud->flags);
        log("ffxDispatch ud->frameTimeDelta: {}", ud->frameTimeDelta);
        log("ffxDispatch ud->jitterOffset: ({}, {})", ud->jitterOffset.x, ud->jitterOffset.y);
        log("ffxDispatch ud->motionVectors: {}null", ud->motionVectors.resource ? "not " : "");
        log("ffxDispatch ud->motionVectorScale: ({}, {})", ud->motionVectorScale.x, ud->motionVectorScale.y);
        log("ffxDispatch ud->output: {}null", ud->output.resource ? "not " : "");
        log("ffxDispatch ud->preExposure: {}", ud->preExposure);
        log("ffxDispatch ud->reactive: {}null", ud->reactive.resource ? "not " : "");
        log("ffxDispatch ud->renderSize: ({}, {})", ud->renderSize.width, ud->renderSize.height);
        log("ffxDispatch ud->reset: {}", ud->reset);
        log("ffxDispatch ud->sharpness: {}", ud->sharpness);
        log("ffxDispatch ud->transparencyAndComposition: {}null", ud->transparencyAndComposition.resource ? "not " : "");
        log("ffxDispatch ud->upscaleSize: ({}, {})", ud->upscaleSize.width, ud->upscaleSize.height);
        log("ffxDispatch ud->viewSpaceToMetersFactor: {}", ud->viewSpaceToMetersFactor);
        break;
    }
    case FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK:
        log("ffxDispatch desc->type: FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK");
        break;
    default:
        log("ffxDispatch desc->type: {}", header->type);
    }

    auto result = _dispatch(context, header);

    log("ffxDispatch result: {}", result);

    return result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    auto logEnv = std::getenv("FFXPROXY_LOG");
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        if (logEnv && *logEnv == '1')
            prepareLogging("ffx_proxy.log");
        else
            prepareLogging(std::nullopt);
        log("--------------");

        _amdDll = LoadLibrary("amd_fidelityfx_dx12.o.dll");

        if (_amdDll != nullptr)
        {
            _createContext = (PfnFfxCreateContext)GetProcAddress(_amdDll, "ffxCreateContext");
            _destroyContext = (PfnFfxDestroyContext)GetProcAddress(_amdDll, "ffxDestroyContext");
            _configure = (PfnFfxConfigure)GetProcAddress(_amdDll, "ffxConfigure");
            _query = (PfnFfxQuery)GetProcAddress(_amdDll, "ffxQuery");
            _dispatch = (PfnFfxDispatch)GetProcAddress(_amdDll, "ffxDispatch");
        }

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        closeLogging();
        FreeLibrary(_amdDll);
        break;
    }

    return TRUE;
}
