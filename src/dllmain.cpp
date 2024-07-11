// dllmain.cpp : Defines the entry point for the DLL application.
#include "log.h"
#include "../include/ffx_api/ffx_api.h"
#include "../include/ffx_api/ffx_upscale.h"
#include <dxgi.h>
#if defined __MINGW64__ || defined __MINGW32__
#include "../include/d3d12.h"
#else
#include <d3d12.h>
#endif
#include "../include/ffx_api/dx12/ffx_api_dx12.h"

static HMODULE _amdDll = nullptr;
static PfnFfxCreateContext _createContext = nullptr;
static PfnFfxDestroyContext _destroyContext = nullptr;
static PfnFfxConfigure _configure = nullptr;
static PfnFfxQuery _query = nullptr;
static PfnFfxDispatch _dispatch = nullptr;

FFX_API_ENTRY ffxReturnCode_t ffxCreateContext(ffxContext* context, ffxCreateContextDescHeader* desc, const ffxAllocationCallbacks* memCb)
{
    if (_createContext == nullptr)
        return FFX_API_RETURN_ERROR;

    log("ffxCreateContext");

    ffxCreateContextDescHeader* header = desc;

    while (header != nullptr)
    {
        if (header->type == FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE)
        {
            log("ffxCreateContext header->type: FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE");
        }
        else if (header->type == FFX_API_DESC_TYPE_OVERRIDE_VERSION)
        {
            log("ffxCreateContext header->type: FFX_API_DESC_TYPE_OVERRIDE_VERSION");
        }
        else if (header->type == FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12)
        {
            log("ffxCreateContext header->type: FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12");
        }
        else
        {
            log("ffxCreateContext header->type: " + std::to_string((uint64_t)header->type));
        }

        header = header->pNext;
    }

    auto result = _createContext(context, desc, memCb);

    log("ffxCreateContext result: " + std::to_string((uint32_t)result));

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxDestroyContext(ffxContext* context, const ffxAllocationCallbacks* memCb)
{
    log("ffxDestroyContext");

    auto result = _destroyContext(context, memCb);

    log("ffxDestroyContext result: " + std::to_string((uint32_t)result));

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxConfigure(ffxContext* context, const ffxConfigureDescHeader* desc)
{
    log("ffxConfigure");

    auto result = _configure(context, desc);

    log("ffxConfigure result: " + std::to_string((uint32_t)result));

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* desc)
{
    log("ffxQuery");

    auto result = _query(context, desc);

    log("ffxQuery result: " + std::to_string((uint32_t)result));

    return result;
}

FFX_API_ENTRY ffxReturnCode_t ffxDispatch(ffxContext* context, const ffxDispatchDescHeader* desc)
{
    log("ffxDispatch");

    if (desc->type == FFX_API_DISPATCH_DESC_TYPE_UPSCALE)
    {
        log("ffxDispatch desc->type: FFX_API_DISPATCH_DESC_TYPE_UPSCALE");
        auto ud = (ffxDispatchDescUpscale*)desc;

        log("ffxDispatch ud->cameraFar: " + std::to_string(ud->cameraFar));
        log("ffxDispatch ud->cameraFovAngleVertical: " + std::to_string(ud->cameraFovAngleVertical));
        log("ffxDispatch ud->cameraNear: " + std::to_string(ud->cameraNear));

        if (ud->color.resource != nullptr)
            log("ffxDispatch ud->color: not null");
        else
            log("ffxDispatch ud->color: null");

        if (ud->commandList != nullptr)
            log("ffxDispatch ud->commandList: not null");
        else
            log("ffxDispatch ud->commandList: null");

        if (ud->depth.resource != nullptr)
            log("ffxDispatch ud->depth: not null");
        else
            log("ffxDispatch ud->depth: null");

        if (ud->enableSharpening)
            log("ffxDispatch ud->enableSharpening: true");
        else
            log("ffxDispatch ud->enableSharpening: false");

        if (ud->exposure.resource != nullptr)
            log("ffxDispatch ud->exposure: not null");
        else
            log("ffxDispatch ud->exposure: null");

        log("ffxDispatch ud->flags: " + std::to_string(ud->flags));
        log("ffxDispatch ud->frameTimeDelta: " + std::to_string(ud->frameTimeDelta));
        log("ffxDispatch ud->jitterOffset: {" + std::to_string(ud->jitterOffset.x) + ", " + std::to_string(ud->jitterOffset.y) + "}");

        if (ud->motionVectors.resource != nullptr)
            log("ffxDispatch ud->motionVectors: not null");
        else
            log("ffxDispatch ud->motionVectors: null");

        log("ffxDispatch ud->motionVectorScale: {" + std::to_string(ud->motionVectorScale.x) + ", " + std::to_string(ud->motionVectorScale.y) + "}");

        if ((ud->output.resource != nullptr))
            log("ffxDispatch ud->output: not null");
        else
            log("ffxDispatch ud->output: null");

        log("ffxDispatch ud->preExposure: " + std::to_string(ud->preExposure));

        if ((ud->reactive.resource != nullptr))
            log("ffxDispatch ud->reactive: not null");
        else
            log("ffxDispatch ud->reactive: null");

        log("ffxDispatch ud->renderSize: {" + std::to_string(ud->renderSize.width) + ", " + std::to_string(ud->renderSize.height) + "}");
        log("ffxDispatch ud->reset: " + std::to_string(ud->reset));
        log("ffxDispatch ud->sharpness: " + std::to_string(ud->sharpness));

        if (ud->transparencyAndComposition.resource != nullptr)
            log("ffxDispatch ud->transparencyAndComposition: not null");
        else
            log("ffxDispatch ud->transparencyAndComposition: null");

        log("ffxDispatch ud->upscaleSize: {" + std::to_string(ud->upscaleSize.width) + ", " + std::to_string(ud->upscaleSize.height) + "}");
        log("ffxDispatch ud->viewSpaceToMetersFactor: " + std::to_string(ud->viewSpaceToMetersFactor));
    }
    else if (desc->type == FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK)
    {
        log("ffxDispatch desc->type: FFX_API_DISPATCH_DESC_TYPE_UPSCALE_GENERATEREACTIVEMASK");
    }
    else
    {
        log("ffxDispatch desc->type: " + std::to_string((uint64_t)desc->type));
    }

    auto result = _dispatch(context, desc);

    log("ffxDispatch result: " + std::to_string((uint32_t)result));

    return result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);

            prepareLogging("fsr31proxy.log");

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

