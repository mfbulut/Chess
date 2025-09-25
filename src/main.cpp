#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <mmdeviceapi.h>
#include <audioclient.h>

#include <dwmapi.h>
#include <math.h>

#define QOI_IMPLEMENTATION
#include "qoi.h"

#include "spritesheet.h"

const char* shader_hlsl = "cbuffer constants : register(b0)\n"
"{\n"
"    float2 rn_screensize;\n"
"    float2 r_texturesize;\n"
"}\n"
"\n"
"struct spritedesc\n"
"{\n"
"    float2 location;\n"
"    float2 size;\n"
"    float2 anchor;\n"
"    float2 position;\n"
"    float2 scale;\n"
"    float  rotation;\n"
"    uint   color;\n"
"};\n"
"\n"
"struct pixeldesc\n"
"{\n"
"    float4 position : SV_POSITION;\n"
"    float2 location : LOC;\n"
"    float4 color    : COL;\n"
"};\n"
"\n"
"StructuredBuffer<spritedesc> spritebatch : register(t0);\n"
"Texture2D                    spritesheet : register(t1);\n"
"\n"
"SamplerState                 aaptsampler : register(s0);\n"
"\n"
"pixeldesc vs(uint spriteid : SV_INSTANCEID, uint vertexid : SV_VERTEXID)\n"
"{\n"
"    spritedesc sprite = spritebatch[spriteid];\n"
"\n"
"    uint2  idx = { vertexid & 2, (vertexid << 1 & 2) ^ 3 };\n"
"\n"
"    float4 piv = float4(0, 0, sprite.size + 1) * sprite.scale.xyxy - (sprite.size * sprite.scale * sprite.anchor).xyxy;\n"
"    float2 pos = float2(piv[idx.x] * cos(sprite.rotation) - piv[idx.y] * sin(sprite.rotation), piv[idx.y] * cos(sprite.rotation) + piv[idx.x] * sin(sprite.rotation)) + sprite.position - 0.5f;\n"
"\n"
"    float4 loc = float4(sprite.location, sprite.location + sprite.size + 1);\n"
"\n"
"    pixeldesc output;\n"
"\n"
"    output.position = float4(pos * rn_screensize - float2(1, -1), 0, 1);\n"
"    output.location = float2(loc[idx.x], loc[idx.y]);\n"
"    output.color    = float4(sprite.color >> uint4(0, 8, 16, 24) & 0xff) / 255;\n"
"\n"
"    return output;\n"
"}\n"
"\n"
"float4 ps(pixeldesc pixel) : SV_TARGET\n"
"{\n"
"    float2 uv = (pixel.location * r_texturesize) + (0.5f * r_texturesize);\n"
"    float4 texColor = spritesheet.Sample(aaptsampler, uv);\n"
"    float4 color = texColor * float4(pixel.color.rgb, pixel.color.a);\n"
"\n"
"    if (color.a == 0) discard;\n"
"\n"
"    return color;\n"
"}";

struct vec2   { float x, y; };
struct color  { unsigned char r, g, b, a; };
struct sprite { vec2 location, size, anchor, position, scale; float rotation; color color; };

#define MAX_SPRITES 65536
sprite* spritebatch;
int spritecount = 0;

struct InputState {
    bool keys_current[256];
    bool keys_previous[256];

    bool mouse_current[3];
    bool mouse_previous[3];

    vec2 mouse_position;
    vec2 mouse_delta;
    float mouse_wheel;
} input = { 0 };

struct TimeState {
    LARGE_INTEGER frequency;
    LARGE_INTEGER start_time;
    LARGE_INTEGER current_time;
    LARGE_INTEGER previous_time;
    float delta_time;
    float total_time;
} time_state = { 0 };

void draw(vec2 location, vec2 size, vec2 position, color col) {
    sprite sprite = {
        location,
        size,
        { 0, 0 },
        position,
        { 1, 1 },
        0,
        col,
    };
    spritebatch[spritecount++] = sprite;
}

bool key_down(int key) {
    return input.keys_current[key];
}

bool key_pressed(int key) {
    return input.keys_current[key] && !input.keys_previous[key];
}

bool key_released(int key) {
    return !input.keys_current[key] && input.keys_previous[key];
}

bool mouse_down(int button) {
    return input.mouse_current[button];
}

bool mouse_pressed(int button) {
    return input.mouse_current[button] && !input.mouse_previous[button];
}

bool mouse_released(int button) {
    return !input.mouse_current[button] && input.mouse_previous[button];
}

vec2 mouse_position() {
    return input.mouse_position;
}

vec2 mouse_delta() {
    return input.mouse_delta;
}

float mouse_wheel() {
    return input.mouse_wheel;
}

float delta_time() {
    return time_state.delta_time;
}

float total_time() {
    return time_state.total_time;
}

#define SAMPLE_RATE 48000

#include "game.cpp"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        case WM_KEYDOWN:
            if (wParam < 256) {
                input.keys_current[wParam] = true;
            }
            return 0;

        case WM_KEYUP:
            if (wParam < 256) {
                input.keys_current[wParam] = false;
            }
            return 0;

        case WM_SYSKEYDOWN:
            if (wParam < 256) {
                input.keys_current[wParam] = true;
            }
            return 0;

        case WM_SYSKEYUP:
            if (wParam < 256) {
                input.keys_current[wParam] = false;
            }
            return 0;

        case WM_LBUTTONDOWN:
            input.mouse_current[0] = true;
            SetCapture(hwnd);
            return 0;

        case WM_LBUTTONUP:
            input.mouse_current[0] = false;
            ReleaseCapture();
            return 0;

        case WM_RBUTTONDOWN:
            input.mouse_current[1] = true;
            SetCapture(hwnd);
            return 0;

        case WM_RBUTTONUP:
            input.mouse_current[1] = false;
            ReleaseCapture();
            return 0;

        case WM_MBUTTONDOWN:
            input.mouse_current[2] = true;
            SetCapture(hwnd);
            return 0;

        case WM_MBUTTONUP:
            input.mouse_current[2] = false;
            ReleaseCapture();
            return 0;

        case WM_MOUSEMOVE:
        {
            vec2 new_pos = { (float)LOWORD(lParam), (float)HIWORD(lParam) };
            input.mouse_delta = { new_pos.x - input.mouse_position.x, new_pos.y - input.mouse_position.y };
            input.mouse_position = new_pos;
            return 0;
        }

        case WM_MOUSEWHEEL:
            input.mouse_wheel = (float)GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    SetProcessDPIAware();

    QueryPerformanceFrequency(&time_state.frequency);
    QueryPerformanceCounter(&time_state.start_time);
    time_state.previous_time = time_state.start_time;
    time_state.delta_time = 0.0f;
    time_state.total_time = 0.0f;

    qoi_desc desc;
    void *spritesheetdata = qoi_decode(spritesheet, 55792, &desc, 4);

    WNDCLASSA wndclass = { 0, WndProc, 0, 0, hInstance, LoadIconA(hInstance, MAKEINTRESOURCE(101)), LoadCursor(NULL, IDC_ARROW), 0, 0, TITLE };

    RegisterClassA(&wndclass);

    HWND window = CreateWindowExA(0, TITLE, TITLE, (WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW) & ~(WS_THICKFRAME | WS_MAXIMIZEBOX), 600, 180, WINDOW_WIDTH, WINDOW_HEIGHT, nullptr, nullptr, hInstance, nullptr);

    BOOL darktitlebar = TRUE;
    DwmSetWindowAttribute(window, 20, &darktitlebar, sizeof(darktitlebar));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

    DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
    swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    swapchaindesc.SampleDesc.Count  = 1;
    swapchaindesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchaindesc.BufferCount       = 2;
    swapchaindesc.OutputWindow      = window;
    swapchaindesc.Windowed          = TRUE;
    swapchaindesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    IDXGISwapChain* swapchain;

    ID3D11Device* device;
    ID3D11DeviceContext* devicecontext;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION, &swapchaindesc, &swapchain, &device, nullptr, &devicecontext);

    swapchain->GetDesc(&swapchaindesc);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3D11Texture2D* framebuffer;

    swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&framebuffer);

    D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {}; // needed for SRGB framebuffer when using FLIP model swap effect (line 39)
    framebufferRTVdesc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

    ID3D11RenderTargetView* framebufferRTV;

    device->CreateRenderTargetView(framebuffer, &framebufferRTVdesc, &framebufferRTV);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    float constants[4] = { 2.0f / swapchaindesc.BufferDesc.Width, -2.0f / swapchaindesc.BufferDesc.Height, 1.0f / desc.width, 1.0f / desc.height };

    D3D11_BUFFER_DESC constantbufferdesc = {};
    constantbufferdesc.ByteWidth = sizeof(constants) + 0xf & 0xfffffff0;
    constantbufferdesc.Usage     = D3D11_USAGE_IMMUTABLE;
    constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

    D3D11_SUBRESOURCE_DATA constantbufferSRD = { constants };

    ID3D11Buffer* constantbuffer;

    device->CreateBuffer(&constantbufferdesc, &constantbufferSRD, &constantbuffer);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_RASTERIZER_DESC rasterizerdesc = { D3D11_FILL_SOLID, D3D11_CULL_NONE }; // allow horizontal/vertical mirroring of sprite using negative scaling values

    ID3D11RasterizerState* rasterizerstate;

    device->CreateRasterizerState(&rasterizerdesc, &rasterizerstate);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_SAMPLER_DESC samplerdesc = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER };

    ID3D11SamplerState* samplerstate;

    device->CreateSamplerState(&samplerdesc, &samplerstate);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_BLEND_DESC blenddesc = { FALSE, FALSE, { TRUE, D3D11_BLEND_ONE, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_BLEND_ZERO, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL } };

    ID3D11BlendState* blendstate;

    device->CreateBlendState(&blenddesc, &blendstate);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* errorBlob;
    ID3DBlob* vertexshaderCSO;

    HRESULT hr = D3DCompile(
        shader_hlsl, strlen(shader_hlsl),        // source & length
        nullptr, nullptr, nullptr,               // optional macros, includes, include handler
        "vs", "vs_5_0",                          // entry point + target profile
        0, 0,                                    // compile flags
        &vertexshaderCSO, &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return 0;
    }

    ID3D11VertexShader* vertexshader;

    device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), 0, &vertexshader);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    ID3DBlob* pixelshaderCSO;

    hr = D3DCompile(
        shader_hlsl, strlen(shader_hlsl),
        nullptr, nullptr, nullptr,
        "ps", "ps_5_0",
        0, 0,
        &pixelshaderCSO, &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return 0;
    }

    ID3D11PixelShader* pixelshader;

    device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), 0, &pixelshader);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    FLOAT clearcolor[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // RGBA

    D3D11_VIEWPORT viewport = { 0, 0, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0, 1 };

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_TEXTURE2D_DESC spritesheetdesc = {};
    spritesheetdesc.Width            = desc.width;
    spritesheetdesc.Height           = desc.height;
    spritesheetdesc.MipLevels        = 1;
    spritesheetdesc.ArraySize        = 1;
    spritesheetdesc.Format           = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    spritesheetdesc.SampleDesc.Count = 1;
    spritesheetdesc.Usage            = D3D11_USAGE_IMMUTABLE;
    spritesheetdesc.BindFlags        = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA spritesheetSRD = {};
    spritesheetSRD.pSysMem     = spritesheetdata;
    spritesheetSRD.SysMemPitch = desc.width * sizeof(UINT);

    ID3D11Texture2D* spritesheet;

    device->CreateTexture2D(&spritesheetdesc, &spritesheetSRD, &spritesheet);

    ID3D11ShaderResourceView* spritesheetSRV;

    device->CreateShaderResourceView(spritesheet, nullptr, &spritesheetSRV);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    D3D11_BUFFER_DESC spritebufferdesc = {};
    spritebufferdesc.ByteWidth           = sizeof(sprite) * MAX_SPRITES;
    spritebufferdesc.Usage               = D3D11_USAGE_DYNAMIC;
    spritebufferdesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    spritebufferdesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    spritebufferdesc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    spritebufferdesc.StructureByteStride = sizeof(sprite);

    ID3D11Buffer* spritebuffer;

    device->CreateBuffer(&spritebufferdesc, nullptr, &spritebuffer);

    D3D11_SHADER_RESOURCE_VIEW_DESC spritebufferSRVdesc = {};
    spritebufferSRVdesc.Format             = DXGI_FORMAT_UNKNOWN;
    spritebufferSRVdesc.ViewDimension      = D3D11_SRV_DIMENSION_BUFFER;
    spritebufferSRVdesc.Buffer.NumElements = MAX_SPRITES;

    ID3D11ShaderResourceView* spritebufferSRV;

    device->CreateShaderResourceView(spritebuffer, &spritebufferSRVdesc, &spritebufferSRV);

    spritebatch = (sprite*)HeapAlloc(GetProcessHeap(), 0, sizeof(sprite) * MAX_SPRITES);


    CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    IMMDeviceEnumerator* deviceEnumerator;

    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&deviceEnumerator));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    IMMDevice* audioDevice;

    deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audioDevice);

    ///////////////////////////////////////////////////////////////////////////////////////////////

    IAudioClient3* audioClient;

    audioDevice->Activate(__uuidof(IAudioClient3), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void**>(&audioClient));

    ///////////////////////////////////////////////////////////////////////////////////////////////

    REFERENCE_TIME defaultPeriod;
    REFERENCE_TIME minimumPeriod;

    audioClient->GetDevicePeriod(&defaultPeriod, &minimumPeriod);

    WAVEFORMATEX* mixFormat;

    audioClient->GetMixFormat(&mixFormat);

    audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, minimumPeriod, 0, mixFormat, nullptr);

    IAudioRenderClient* audioRenderClient;

    audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&audioRenderClient));

    UINT32 bufferSize;

    audioClient->GetBufferSize(&bufferSize);

    HANDLE bufferReady = CreateEventA(nullptr, FALSE, FALSE, nullptr);

    audioClient->SetEventHandle(bufferReady);

    audioClient->Start();

    game_init();

    while (true)
    {
        MSG msg;

        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if(msg.message == WM_QUIT) return 0;
            if(msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) return 0;
            DispatchMessageA(&msg);
        }

        if (WaitForSingleObject(bufferReady, 0) == WAIT_OBJECT_0)
        {
            UINT32 bufferPadding;

            audioClient->GetCurrentPadding(&bufferPadding);

            UINT32 frameCount = bufferSize - bufferPadding;
            float* buffer;

            audioRenderClient->GetBuffer(frameCount, reinterpret_cast<BYTE**>(&buffer));

            game_audio(buffer, frameCount);

            audioRenderClient->ReleaseBuffer(frameCount, 0);
        }

        spritecount = 0;

        QueryPerformanceCounter(&time_state.current_time);
        time_state.delta_time = (float)(time_state.current_time.QuadPart - time_state.previous_time.QuadPart) / (float)time_state.frequency.QuadPart;
        time_state.total_time = (float)(time_state.current_time.QuadPart - time_state.start_time.QuadPart) / (float)time_state.frequency.QuadPart;

        if (time_state.delta_time > 0.05f) {
            time_state.delta_time = 0.05f;
        }

        time_state.previous_time = time_state.current_time;

        game_update();

        memcpy(input.keys_previous, input.keys_current, sizeof(input.keys_current));
        memcpy(input.mouse_previous, input.mouse_current, sizeof(input.mouse_current));

        input.mouse_wheel = 0.0f;

        ///////////////////////////////////////////////////////////////////////////////////////////

        D3D11_MAPPED_SUBRESOURCE spritebufferMSR;

        devicecontext->Map(spritebuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &spritebufferMSR);
        {
            memcpy(spritebufferMSR.pData, spritebatch, sizeof(sprite) * spritecount);
        }
        devicecontext->Unmap(spritebuffer, 0);

        devicecontext->OMSetRenderTargets(1, &framebufferRTV, nullptr);

        devicecontext->ClearRenderTargetView(framebufferRTV, clearcolor);

        devicecontext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

        devicecontext->VSSetShader(vertexshader, nullptr, 0);
        devicecontext->VSSetShaderResources(0, 1, &spritebufferSRV);
        devicecontext->VSSetConstantBuffers(0, 1, &constantbuffer);

        devicecontext->RSSetViewports(1, &viewport);
        devicecontext->RSSetState(rasterizerstate);

        devicecontext->PSSetShader(pixelshader, nullptr, 0);
        devicecontext->PSSetShaderResources(1, 1, &spritesheetSRV);
        devicecontext->PSSetConstantBuffers(0, 1, &constantbuffer);
        devicecontext->PSSetSamplers(0, 1, &samplerstate);

        devicecontext->OMSetBlendState(blendstate, nullptr, 0xffffffff);

        devicecontext->DrawInstanced(4, spritecount, 0, 0);

        swapchain->Present(1, 0);
    }
}