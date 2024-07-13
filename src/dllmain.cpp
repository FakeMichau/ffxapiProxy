#include "log.h"
#include <dxgi.h>
#include <d3d12.h>
#include <ffx_api/ffx_api.hpp>
#include <ffx_api/ffx_upscale.h>
#include <ffx_api/dx12/ffx_api_dx12.h>

struct Config
{
    std::vector<const char *> versionNames;
    std::vector<uint64_t> versionIds;
    u_int upscalerIndex = 0; // assuming 1 == 2.3.2
    bool debugView = true;
} config;

static HMODULE _amdDll = nullptr;
static PfnFfxCreateContext _createContext = nullptr;
static PfnFfxDestroyContext _destroyContext = nullptr;
static PfnFfxConfigure _configure = nullptr;
static PfnFfxQuery _query = nullptr;
static PfnFfxDispatch _dispatch = nullptr;

FFX_API_ENTRY ffxReturnCode_t ffxCreateContext(ffxContext *context, ffxCreateContextDescHeader *desc, const ffxAllocationCallbacks *memCb)
{
    if (_createContext == nullptr)
        return FFX_API_RETURN_ERROR;

    log("ffxCreateContext");

    for (const auto *it = desc->pNext; it; it = it->pNext)
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
    override.header.pNext = desc->pNext;
    desc->pNext = &override.header;

    auto result = _createContext(context, desc, memCb);

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

FFX_API_ENTRY ffxReturnCode_t ffxQuery(ffxContext *context, ffxQueryDescHeader *desc)
{
    log("ffxQuery");

    auto result = _query(context, desc);

    log("ffxQuery result: {}", result);

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxDispatch(ffxContext *context, const ffxDispatchDescHeader *desc)
{
    log("ffxDispatch");

    switch (desc->type)
    {
    case FFX_API_DISPATCH_DESC_TYPE_UPSCALE:
    {
        auto ud = (ffxDispatchDescUpscale *)desc;
        log("ffxDispatch desc->type: FFX_API_DISPATCH_DESC_TYPE_UPSCALE");
        log("ffxDispatch ud->cameraFar: {}", ud->cameraFar);
        log("ffxDispatch ud->cameraFovAngleVertical: {}", ud->cameraFovAngleVertical);
        log("ffxDispatch ud->cameraNear: {}", ud->cameraNear);
        log("ffxDispatch ud->color: {}null", ud->color.resource ? "not ": "");
        log("ffxDispatch ud->commandList: {}null", ud->commandList ? "not ": "");
        log("ffxDispatch ud->depth: {}null", ud->depth.resource ? "not ": "");
        log("ffxDispatch ud->enableSharpening: {}", ud->enableSharpening);
        log("ffxDispatch ud->exposure: {}null", ud->exposure.resource ? "not ": "");
        log("ffxDispatch ud->flags: {}", ud->flags);
        log("ffxDispatch ud->frameTimeDelta: {}", ud->frameTimeDelta);
        log("ffxDispatch ud->jitterOffset: ({}, {})", ud->jitterOffset.x, ud->jitterOffset.y);
        log("ffxDispatch ud->motionVectors: {}null", ud->motionVectors.resource ? "not ": "");
        log("ffxDispatch ud->motionVectorScale: ({}, {})", ud->motionVectorScale.x, ud->motionVectorScale.y);
        log("ffxDispatch ud->output: {}null", ud->output.resource ? "not ": "");
        log("ffxDispatch ud->preExposure: {}", ud->preExposure);
        log("ffxDispatch ud->reactive: {}null", ud->reactive.resource ? "not ": "");
        log("ffxDispatch ud->renderSize: ({}, {})", ud->renderSize.width, ud->renderSize.height);
        log("ffxDispatch ud->reset: {}", ud->reset);
        log("ffxDispatch ud->sharpness: {}", ud->sharpness);
        log("ffxDispatch ud->transparencyAndComposition: {}null", ud->transparencyAndComposition.resource ? "not ": "");
        log("ffxDispatch ud->upscaleSize: ({}, {})", ud->upscaleSize.width, ud->upscaleSize.height);
        log("ffxDispatch ud->viewSpaceToMetersFactor: {}", ud->viewSpaceToMetersFactor);

        if (config.debugView)
            ud->flags = FFX_UPSCALE_FLAG_DRAW_DEBUG_VIEW;

        break;
    }
    case FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK:
        log("ffxDispatch desc->type: FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK");
        break;
    default:
        log("ffxDispatch desc->type: {}", desc->type);
    }

    auto result = _dispatch(context, desc);

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
