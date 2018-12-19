#include "suki.h"

#define HANDMADE_MATH_IMPLEMENTATION
#include "..\kaizen\HandmadeMath.h" // TODO delete

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

// Allow compilation with old win32 SDK. MinGW doesn't have default
// _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

struct {
    MSG msg;
    HWND hwnd;
    WNDCLASSEX wc;
    char *title;
    int64_t time;
    int64_t ticks_per_second;
    uint32_t window_width;
    uint32_t window_height;
#if defined(SUKI_D3D9)
    struct {
        IDirect3D9 *info;
        IDirect3DDevice9 *device;
        D3DPRESENT_PARAMETERS parameter;
        LPDIRECT3DVERTEXBUFFER9 v_buffer;
        LPDIRECT3DINDEXBUFFER9 i_buffer;
        LPDIRECT3DTEXTURE9 t_buffer;
    } d3d9;
#elif defined(SUKI_D3D11)
    struct {
        ID3D11Device *device;
        ID3D11DeviceContext *device_context;
        IDXGISwapChain *swap_chain;
        ID3D11RenderTargetView *render_target_view;
        ID3D11Buffer *v_buffer;
        ID3D11Buffer *i_buffer;
        ID3D11InputLayout *input_layout;
        ID3D11VertexShader *vertex_shader;
        ID3D11Buffer *vertex_constant_buffer;
        ID3D11PixelShader *pixel_shader;
        ID3D10Blob *vertex_shader_blob;
        ID3D10Blob *pixel_shader_blob;
        ID3D11SamplerState *font_sampler;
        ID3D11ShaderResourceView *fontTexture_view;
        ID3D11RasterizerState *rasterizer_state;
        ID3D11BlendState* blend_state;
        ID3D11DepthStencilState* depth_stencil_state;
    } d3d11;
#endif
} g_suki_win32;

typedef struct // ????
{
    float mvp[4][4];
} VERTEX_CONSTANT_BUFFER;

static bool win32_update_mouse_cursor(void);
static void win32_update_mouse_position(void);
extern void suki_reshape_view(int32_t x_size, int32_t y_size);
extern void suki_button_set(int32_t user_id, int32_t id, bool state, int32_t character);

#if defined(SUKI_D3D9)
#define CUSTOM_FVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1)
static void d3d9_draw_data(Suki_Draw_Data *draw_data)
{
    Suki_Vertex *v_dest = NULL;
    Suki_Index *i_dest = NULL;

    IDirect3DDevice9_CreateVertexBuffer(g_suki_win32.d3d9.device, SUKI_NUM_VERTEX_MAX*sizeof(Suki_Vertex), 0, CUSTOM_FVF, D3DPOOL_MANAGED, &g_suki_win32.d3d9.v_buffer, NULL);

    IDirect3DDevice9_CreateIndexBuffer(g_suki_win32.d3d9.device, SUKI_NUM_INDEXES_MAX*sizeof(Suki_Index), 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &g_suki_win32.d3d9.i_buffer, NULL);

    // lock g_v_buffer and load the vertices into it
    if (IDirect3DIndexBuffer9_Lock(g_suki_win32.d3d9.v_buffer, 0, 0, (void **)&v_dest, 0) != D3D_OK)
        assert(0);
    if (IDirect3DIndexBuffer9_Lock(g_suki_win32.d3d9.i_buffer, 0, 0, (void **)&i_dest, 0) != D3D_OK)
        assert(0);

    {
        for (uint32_t ii = 0; ii < draw_data->num_vertices; ii++)
        {
            v_dest->x = draw_data->vertices[ii].x;
            v_dest->y = draw_data->vertices[ii].y;
            v_dest->z = draw_data->vertices[ii].z;
            v_dest->rhw = draw_data->vertices[ii].rhw;
            v_dest->color = (draw_data->vertices[ii].color & 0xFF00FF00) | ((draw_data->vertices[ii].color & 0x00FF0000) >> 16) |  ((draw_data->vertices[ii].color & 0x000000FF) << 16); // RGBA --> ARGB for DirectX9
            v_dest->u = draw_data->vertices[ii].u;
            v_dest->v = draw_data->vertices[ii].v;
            v_dest++;
        }
        memcpy(i_dest, &draw_data->indexes[0], sizeof(Suki_Index) * draw_data->num_indexes);
    }

    IDirect3DIndexBuffer9_Unlock(g_suki_win32.d3d9.v_buffer);
    IDirect3DIndexBuffer9_Unlock(g_suki_win32.d3d9.i_buffer);
    IDirect3DDevice9_SetStreamSource(g_suki_win32.d3d9.device, 0, g_suki_win32.d3d9.v_buffer, 0, sizeof(Suki_Vertex));
    IDirect3DDevice9_SetIndices(g_suki_win32.d3d9.device, g_suki_win32.d3d9.i_buffer);
    IDirect3DDevice9_SetFVF(g_suki_win32.d3d9.device, CUSTOM_FVF);

    IDirect3DDevice9_DrawIndexedPrimitive(g_suki_win32.d3d9.device, D3DPT_TRIANGLELIST, 0, 0, draw_data->num_vertices, 0, draw_data->num_indexes/3);
}

static int32_t d3d9_create_device()
{
    assert(g_suki_win32.d3d9.info);
    IDirect3DDevice9 *d3d_device;
    if (IDirect3D9_CreateDevice(g_suki_win32.d3d9.info, D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_suki_win32.hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_suki_win32.d3d9.parameter, &d3d_device) != D3D_OK) {
        IDirect3D9_Release(g_suki_win32.d3d9.info);
        UnregisterClass(_T(g_suki_win32.title), g_suki_win32.wc.hInstance);
        assert(0);
        return -1;
    }
    g_suki_win32.d3d9.device = d3d_device;
    return 0;
}

static void d3d9_render()
{
    HRESULT result;
    Suki_Draw_Data *draw_data = suki_get_draw_data();
    assert(g_suki_win32.d3d9.device);

    uint32_t r = (draw_data->color_background & 0x000000FF) >> 0;
    uint32_t g = (draw_data->color_background & 0x0000FF00) >> 8;
    uint32_t b = (draw_data->color_background & 0x00FF0000) >> 16;
    uint32_t a = (draw_data->color_background & 0xFF000000) >> 24;
    D3DCOLOR bg_color = D3DCOLOR_RGBA(r, g, b, a);

    IDirect3DDevice9_Clear(g_suki_win32.d3d9.device, 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, bg_color, 1.0f, 0);
    if (IDirect3DDevice9_BeginScene(g_suki_win32.d3d9.device) >= 0) {
        d3d9_draw_data(draw_data);
        IDirect3DDevice9_EndScene(g_suki_win32.d3d9.device);
    }
    result = IDirect3DDevice9_Present(g_suki_win32.d3d9.device, NULL, NULL, NULL, NULL);
    // Handle loss of D3D9 device
    if (result == D3DERR_DEVICELOST && IDirect3DDevice9_TestCooperativeLevel(g_suki_win32.d3d9.device) == D3DERR_DEVICENOTRESET) { // @Incomplete :: Reset Texture when we will implement it
        IDirect3DDevice9_Reset(g_suki_win32.d3d9.device, &g_suki_win32.d3d9.parameter);
        g_suki_win32.d3d9.device = NULL;
    }
}

int32_t d3d9_initialize(void)
{
    // TIMED_FUNCTION();
    D3DPRESENT_PARAMETERS d3d_parameter;
    IDirect3D9 *d3d;
    if ((d3d = Direct3DCreate9(D3D_SDK_VERSION)) == NULL) {
        UnregisterClass(_T(g_suki_win32.title), g_suki_win32.wc.hInstance);
        return -1;
    }
    g_suki_win32.d3d9.info = d3d;
    ZeroMemory(&d3d_parameter, sizeof(d3d));
    d3d_parameter.Windowed = TRUE;
    d3d_parameter.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3d_parameter.BackBufferFormat = D3DFMT_UNKNOWN;
    d3d_parameter.EnableAutoDepthStencil = TRUE;
    d3d_parameter.AutoDepthStencilFormat = D3DFMT_D16;
    d3d_parameter.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // Present with vsync
    //d3d_parameter.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE; // Present without vsync, maximum unthrottled framerate
    g_suki_win32.d3d9.parameter = d3d_parameter;
    d3d9_create_device();

    return 0;
}
#elif defined(SUKI_D3D11)

static void d3d11_draw_data(Suki_Draw_Data *draw_data)
{
    ID3D11DeviceContext* ctx = g_suki_win32.d3d11.device_context;
    // Create and grow vertex/index buffers if needed
    if (!g_suki_win32.d3d11.v_buffer)
    {
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = 32 * sizeof(Suki_Vertex);
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        if (ID3D11Device_CreateBuffer(g_suki_win32.d3d11.device, &desc, NULL, &g_suki_win32.d3d11.v_buffer) != S_OK)
            assert(0);
    }
    if (!g_suki_win32.d3d11.i_buffer)
    {
        D3D11_BUFFER_DESC desc;
        memset(&desc, 0, sizeof(D3D11_BUFFER_DESC));
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = 32 * sizeof(Suki_Index);
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if (ID3D11Device_CreateBuffer(g_suki_win32.d3d11.device, &desc, NULL, &g_suki_win32.d3d11.i_buffer) != S_OK)
            assert(0);
    }

    D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
    if (ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)g_suki_win32.d3d11.v_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource) != S_OK)
        assert(0);
    if (ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)g_suki_win32.d3d11.i_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource) != S_OK)
        assert(0);

    Suki_Vertex* v_dest = (Suki_Vertex*)vtx_resource.pData;
    Suki_Index* i_dest = (Suki_Index*)idx_resource.pData;

    memcpy(v_dest, &draw_data->vertices[0], sizeof(Suki_Vertex) * draw_data->num_vertices);
    memcpy(i_dest, &draw_data->indexes[0], sizeof(Suki_Index) * draw_data->num_indexes);

    ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)g_suki_win32.d3d11.v_buffer, 0);
    ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)g_suki_win32.d3d11.i_buffer, 0);

#if 1
    // Setup orthographic projection matrix into our constant buffer
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). 
    {
        D3D11_MAPPED_SUBRESOURCE mapped_resource;
        if (ID3D11DeviceContext_Map(ctx, (ID3D11Resource *)g_suki_win32.d3d11.vertex_constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource) != S_OK)
            assert(0);
        VERTEX_CONSTANT_BUFFER* constant_buffer = (VERTEX_CONSTANT_BUFFER*)mapped_resource.pData;
        float _near = 1.f;
        float _far = 10.f;
#if 0
        float L = 0.0f;
        float R = 0.0f + (float)g_suki_win32.window_width;
        float T = 0.0f;
        float B = 0.0f + (float)g_suki_win32.window_height;
        float mvp[4][4] = { 0.0f };
        mvp[0][0] = 2.0f/(R-L);
        mvp[1][1] = 2.0f/(T-B);
        // mvp[2][2] = 0.5f;
        mvp[2][2] = 2.0f / (_near - _far);
        mvp[3][0] = (R+L)/(L-R);
        mvp[3][1] = (T+B)/(B-T);
        // mvp[3][2] = 0.5f;
        mvp[3][2] = (_far + _near) / (_near - _far);
        mvp[3][3] = 1.0f;
        memcpy(&constant_buffer->mvp, mvp, sizeof(mvp));
#else
#if 1
        // proj
        static float fov = 10.0f;
        fov += 0.001f;
        hmm_mat4 proj = HMM_Perspective(fov, (float)g_suki_win32.window_width/(float)g_suki_win32.window_height, 0.01, 10.0f);
        hmm_mat4 view = HMM_LookAt(HMM_Vec3(0.0f, 1.5f, 8.0f), HMM_Vec3(0.5f, 0.0f, 0.0f), HMM_Vec3(0.0f, 1.0f, 0.0f));
        hmm_mat4 view_proj = HMM_MultiplyMat4(proj, view);
        static float rx = 0.0f, ry = 0.0f;
        rx += 1.20f;
        ry += 1.10f;
        hmm_mat4 rxm = HMM_Rotate(rx, HMM_Vec3(1.0f, 0.0f, 0.0f));
        hmm_mat4 rym = HMM_Rotate(ry, HMM_Vec3(0.0f, 1.0f, 0.0f));
        hmm_mat4 model = HMM_MultiplyMat4(rxm, rym);
        hmm_mat4 mvp = HMM_MultiplyMat4(view_proj, model);
        memcpy(&constant_buffer->mvp, &mvp, sizeof(mvp));
#else
        mvp[3][3] = 0.0f;
        mvp[0][0] = 1.0f;
        mvp[1][1] = 1.0f;
        mvp[2][2] = 1.0f;
        mvp[3][3] = 1.0f;
#endif
#endif
        ID3D11DeviceContext_Unmap(ctx, (ID3D11Resource *)g_suki_win32.d3d11.vertex_constant_buffer, 0);
    }
#endif

   // Setup viewport
    D3D11_VIEWPORT vp;
    memset(&vp, 0, sizeof(D3D11_VIEWPORT));
    vp.Width = (float)g_suki_win32.window_width;
    vp.Height = (float)g_suki_win32.window_height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = vp.TopLeftY = 0;
    ID3D11DeviceContext_RSSetViewports(ctx, 1, &vp);

    // Bind shader and vertex buffers
    unsigned int stride = sizeof(Suki_Vertex);
    unsigned int offset = 0;

    ID3D11DeviceContext_IASetInputLayout(ctx, g_suki_win32.d3d11.input_layout);
    ID3D11DeviceContext_IASetVertexBuffers(ctx, 0, 1, &g_suki_win32.d3d11.v_buffer, &stride, &offset);
    ID3D11DeviceContext_IASetIndexBuffer(ctx, g_suki_win32.d3d11.i_buffer, DXGI_FORMAT_R16_UINT, 0);
    ID3D11DeviceContext_IASetPrimitiveTopology(ctx, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(ctx, g_suki_win32.d3d11.vertex_shader, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(ctx, 0, 1, &g_suki_win32.d3d11.vertex_constant_buffer);
    ID3D11DeviceContext_PSSetShader(ctx, g_suki_win32.d3d11.pixel_shader, NULL, 0);

    // Setup render state
    const float blend_factor[4] = { 0.f, 0.f, 0.f, 0.f };
    ID3D11DeviceContext_OMSetDepthStencilState(ctx, g_suki_win32.d3d11.depth_stencil_state, 0);
    ID3D11DeviceContext_OMSetBlendState(ctx, g_suki_win32.d3d11.blend_state, blend_factor, 0xffffffff);
    ID3D11DeviceContext_RSSetState(ctx, g_suki_win32.d3d11.rasterizer_state);

    // ID3D11DeviceContext_PSSetShaderResources(ctx, 0, 1, NULL); // when there is a texture
    ID3D11DeviceContext_DrawIndexed(g_suki_win32.d3d11.device_context, draw_data->num_indexes, 0, 0);
}

static void d3d11_render()
{
    Suki_Draw_Data *draw_data = suki_get_draw_data();

    float r = (float)((draw_data->color_background & 0x000000FF) >> 0) / 255.f;
    float g = (float)((draw_data->color_background & 0x0000FF00) >> 8) / 255.f;
    float b = (float)((draw_data->color_background & 0x00FF0000) >> 16) / 255.f;
    float a = (float)((draw_data->color_background & 0xFF000000) >> 24) / 255.f;
    float color_background[4] = {0};
    color_background[0] = r;
    color_background[1] = g;
    color_background[2] = b;
    color_background[2] = a;

    ID3D11DeviceContext_OMSetRenderTargets(g_suki_win32.d3d11.device_context, 1, &g_suki_win32.d3d11.render_target_view, NULL);
    ID3D11DeviceContext_ClearRenderTargetView(g_suki_win32.d3d11.device_context, g_suki_win32.d3d11.render_target_view, color_background);
    d3d11_draw_data(draw_data);
    if (IDXGISwapChain_Present(g_suki_win32.d3d11.swap_chain, 0, 0) != S_OK)
        assert(0);
}

static bool d3d11_create_device_object()
{
    assert(g_suki_win32.d3d11.device);
    {
#if 0
        static const char* vertexShader =
            "cbuffer vertexBuffer : register(b0) \
            {\
            float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
            float2 pos : POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
            PS_INPUT output;\
            output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
            output.col = input.col;\
            output.uv  = input.uv;\
            return output;\
            }";
#else
        static const char* vertexShader =
            "cbuffer vertexBuffer : register(b0) \
            {\
            float4x4 ProjectionMatrix; \
            };\
            struct VS_INPUT\
            {\
            float4 pos : POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
            PS_INPUT output;\
            output.pos = mul( ProjectionMatrix, input.pos);\
            output.col = input.col;\
            output.uv  = input.uv;\
            return output;\
            }";
#endif

        if (D3DCompile(vertexShader, strlen(vertexShader), NULL, NULL, NULL, "main", "vs_4_0", 0, 0, &g_suki_win32.d3d11.vertex_shader_blob, NULL) != S_OK)
            assert(0);
        if (g_suki_win32.d3d11.vertex_shader_blob == NULL) // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            assert(0);
        if (ID3D11Device_CreateVertexShader(g_suki_win32.d3d11.device, (DWORD*)ID3D10Blob_GetBufferPointer(g_suki_win32.d3d11.vertex_shader_blob), ID3D10Blob_GetBufferSize(g_suki_win32.d3d11.vertex_shader_blob), NULL, &g_suki_win32.d3d11.vertex_shader) != S_OK)
            assert(0);

        // Create the input layout
        D3D11_INPUT_ELEMENT_DESC local_layout[] = 
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,   0, (size_t)(&((Suki_Vertex*)0)->x /* for vec3 xy */),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((Suki_Vertex*)0)->u /* for vec2 uv */),  D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((Suki_Vertex*)0)->color), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        if (ID3D11Device_CreateInputLayout(g_suki_win32.d3d11.device, local_layout, 3, ID3D10Blob_GetBufferPointer(g_suki_win32.d3d11.vertex_shader_blob), ID3D10Blob_GetBufferSize(g_suki_win32.d3d11.vertex_shader_blob), &g_suki_win32.d3d11.input_layout) != S_OK)
            assert(0);

        // Create the constant buffer
        {
            D3D11_BUFFER_DESC desc;
            desc.ByteWidth = sizeof(VERTEX_CONSTANT_BUFFER);
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            desc.MiscFlags = 0;
            if (ID3D11Device_CreateBuffer(g_suki_win32.d3d11.device, &desc, NULL, &g_suki_win32.d3d11.vertex_constant_buffer))
                assert(0);
        }
    }

    // Create the pixel shader
    {
        static const char* pixelShader =
            "struct PS_INPUT\
            {\
            float4 pos : SV_POSITION;\
            float4 col : COLOR0;\
            float2 uv  : TEXCOORD0;\
            };\
            sampler sampler0;\
            Texture2D texture0;\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
            float4 out_col = input.col; \
            return out_col; \
            }";
            // float4 out_col = input.col * texture0.Sample(sampler0, input.uv);

        if (D3DCompile(pixelShader, strlen(pixelShader), NULL, NULL, NULL, "main", "ps_4_0", 0, 0, &g_suki_win32.d3d11.pixel_shader_blob, NULL))
            assert(0);

        if (g_suki_win32.d3d11.pixel_shader_blob == NULL)  // NB: Pass ID3D10Blob* pErrorBlob to D3DCompile() to get error showing in (const char*)pErrorBlob->GetBufferPointer(). Make sure to Release() the blob!
            assert(0);
        if (ID3D11Device_CreatePixelShader(g_suki_win32.d3d11.device, (DWORD*)ID3D10Blob_GetBufferPointer(g_suki_win32.d3d11.pixel_shader_blob), ID3D10Blob_GetBufferSize(g_suki_win32.d3d11.pixel_shader_blob), NULL, &g_suki_win32.d3d11.pixel_shader) != S_OK)
            assert(0);
    }

    // Create the blending setup
    {
        D3D11_BLEND_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.AlphaToCoverageEnable = false;
        desc.RenderTarget[0].BlendEnable = true;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (ID3D11Device_CreateBlendState(g_suki_win32.d3d11.device, &desc, &g_suki_win32.d3d11.blend_state) != S_OK)
            assert(0);
    }

    // Create the rasterizer state
    {
        D3D11_RASTERIZER_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.ScissorEnable = true;
        desc.DepthClipEnable = true;
        if (ID3D11Device_CreateRasterizerState(g_suki_win32.d3d11.device, &desc, &g_suki_win32.d3d11.rasterizer_state) != S_OK)
            assert(0);
    }

    // Create depth-stencil State
    {
        D3D11_DEPTH_STENCIL_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        // desc.DepthEnable = true;
        // desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        // desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        // desc.StencilEnable = false;
        // desc.FrontFace.StencilFailOp = desc.FrontFace.StencilDepthFailOp = desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        // desc.FrontFace.StencilFunc = D3D11_COMPARISON_LESS_EQUAL;
        // desc.BackFace = desc.FrontFace;
        desc.DepthEnable = TRUE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
        desc.StencilEnable = TRUE;
        desc.StencilReadMask = 0;
        desc.StencilWriteMask = 0;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        if (ID3D11Device_CreateDepthStencilState(g_suki_win32.d3d11.device, &desc, &g_suki_win32.d3d11.depth_stencil_state) != S_OK)
            assert(0);
    }
    return true;
}

void d3d11_create_render_target()
{
    assert(g_suki_win32.d3d11.swap_chain);
    assert(g_suki_win32.d3d11.device);
    ID3D11Resource* back_buffer;
    IDXGISwapChain_GetBuffer(g_suki_win32.d3d11.swap_chain, 0, &IID_ID3D11Texture2D, (LPVOID*)&back_buffer);

    if (ID3D11Device_CreateRenderTargetView(g_suki_win32.d3d11.device, back_buffer, NULL, &g_suki_win32.d3d11.render_target_view) != S_OK)
        assert(0);

    if (ID3D11Texture2D_Release(back_buffer) != S_OK)
        assert(0);
}

void d3d11_cleanup_render_target()
{
    if (g_suki_win32.d3d11.render_target_view) {
        ID3D11RenderTargetView_Release(g_suki_win32.d3d11.render_target_view);
        g_suki_win32.d3d11.render_target_view = NULL;
    }
}

void d3d11_cleanup_device()
{
    d3d11_cleanup_render_target();
    if (g_suki_win32.d3d11.swap_chain) {
        IDXGISwapChain_Release(g_suki_win32.d3d11.swap_chain);
        g_suki_win32.d3d11.swap_chain = NULL;
    }
    if (g_suki_win32.d3d11.device_context) {
        ID3D11DeviceContext_Release(g_suki_win32.d3d11.device_context);
        g_suki_win32.d3d11.device_context = NULL;
    }
    if (g_suki_win32.d3d11.device) {
        ID3D11Device_Release(g_suki_win32.d3d11.device);
        g_suki_win32.d3d11.device = NULL;
    }
}

static int32_t d3d11_create_device()
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_suki_win32.hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_suki_win32.d3d11.swap_chain, &g_suki_win32.d3d11.device, &featureLevel, &g_suki_win32.d3d11.device_context) != S_OK)
        assert(0);

    d3d11_create_render_target();
    return 0;
}

bool d3d11_initialize()
{
    d3d11_create_device();
    return true;
}

static void d3d11_new_frame()
{
    d3d11_create_device_object();
}
#endif

static LRESULT WINAPI window_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Suki_Input *input;
    static bool control, shift;
    assert(hWnd);
    input = suki_get_input_state();
    switch (msg)
    {
        case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: {
            int button = 0;
            if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) button = 0;
            if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) button = 1;
            if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) button = 2;
            input->mouse_down[button] = true;
            if (/* !ImGui::IsAnyMouseDown() && */ GetCapture() == NULL)
                SetCapture(hWnd);
            return 0;
        }
        case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP: {
            int button = 0;
            if (msg == WM_LBUTTONUP) button = 0;
            if (msg == WM_RBUTTONUP) button = 1;
            if (msg == WM_MBUTTONUP) button = 2;
            input->mouse_down[button] = false;
            if (/* !ImGui::IsAnyMouseDown() && */ GetCapture() == hWnd)
                ReleaseCapture();
            return 0;
        }
        case WM_MOUSEWHEEL: {
            input->mouse_wheel_w += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
            printf("wParam = %d\n", (short)(wParam / (256*256)));
            return 0;
        }
        case WM_MOUSEHWHEEL: {
            input->mouse_wheel_h += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
            return 0;
        }
        case WM_KEYUP: {
            if(wParam == VK_CONTROL)
                control = false;
            if(wParam == VK_SHIFT)
                shift = false;
            suki_button_set(0, (int32_t)wParam, false, (int32_t)lParam);
            break;
        }
        case WM_KEYDOWN: {
            if(wParam == VK_CONTROL)
                control = true;
            if(wParam == VK_SHIFT)
                shift = true;
            suki_button_set(0, (int32_t)wParam, true, (int32_t)lParam);
            break;
        }
        case WM_SYSKEYUP: {
            suki_button_set(0, (int32_t)wParam, false, (int32_t)lParam);
            break;
        }
        case WM_SYSKEYDOWN: {
            suki_button_set(0, (int32_t)wParam, true, (int32_t)lParam);
            break;
        }
        case WM_CHAR: {
            suki_button_set(0, -1, TRUE, (int32_t)wParam);
            break;
        }
        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT && win32_update_mouse_cursor())
                return 1;
            return 0;
        }
        case WM_SIZE: {
            int32_t size_x, size_y;
            size_x = LOWORD(lParam);
            size_y = HIWORD(lParam);
            suki_reshape_view(size_x, size_y);
            return 0;
        }
        case WM_SYSCOMMAND: {
            if ((wParam & 0xfff0) == SC_KEYMENU) { // Disable ALT application menu
                return 0;
            }
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
        }
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void win32_initialize(int32_t x, int32_t y, const char *title)
{
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = window_proc;
    wc.cbClsExtra = 0L;
    wc.cbWndExtra = 0L;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T(title);
    wc.hIconSm = NULL;

    RegisterClassEx(&wc);
    g_suki_win32.wc = wc;

    HWND hwnd = CreateWindow( _T(title), _T(title), WS_OVERLAPPEDWINDOW, 100, 100, x, y, NULL, NULL, wc.hInstance, NULL);
    g_suki_win32.hwnd = hwnd;
    g_suki_win32.window_width = x;
    g_suki_win32.window_height = y;

    // time
    if (!QueryPerformanceFrequency((LARGE_INTEGER *)&g_suki_win32.ticks_per_second))
        assert(0);
    if (!QueryPerformanceCounter((LARGE_INTEGER *)&g_suki_win32.time))
        assert(0);

    // prepare the main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    g_suki_win32.msg = msg;
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
}


void d3d_draw_data(Suki_Draw_Data *draw_data)
{
#if defined(SUKI_D3D9)
    d3d9_draw_data(draw_data);
#elif defined(SUKI_D3D11)
    d3d11_draw_data(draw_data);
#endif
}

void d3d_render()
{
#if defined(SUKI_D3D9)
    d3d9_render();
#elif defined(SUKI_D3D11)
    d3d11_render();
#endif
}

int32_t d3d_initialize(void)
{
#if defined(SUKI_D3D9)
    d3d9_initialize();
#elif defined(SUKI_D3D11)
    d3d11_initialize();
#endif
    return 0;
}

bool win32_is_running()
{
    return g_suki_win32.msg.message != WM_QUIT;
}

static bool win32_update_mouse_cursor(void)
{
    Suki_Input *input;
    input = suki_get_input_state();

    if (0) { // @Incomplete // TODO
        // hide cursor
        SetCursor(NULL);
    } else {
        // Show OS mouse cursor
        // @Incomplete
        // LPTSTR win32_cursor = IDC_ARROW;
        //LPTSTR win32_cursor = IDC_IBEAM;
        //LPTSTR win32_cursor = IDC_SIZEALL;
        LPTSTR win32_cursor = IDC_SIZEWE;
        //LPTSTR win32_cursor = IDC_SIZENS;
        //LPTSTR win32_cursor = IDC_SIZENESW;
        //LPTSTR win32_cursor = IDC_SIZENWSE;
        SetCursor(LoadCursor(NULL, win32_cursor));
    }
    return true;
}

static void win32_update_mouse_position(void)
{
    Suki_Input *input;
    input = suki_get_input_state();
    // Set mouse position
    input->mouse_position.x = -FLT_MAX;
    input->mouse_position.y = -FLT_MAX;
    POINT pos;
    if (GetActiveWindow() == g_suki_win32.hwnd && GetCursorPos(&pos)) {
        if (ScreenToClient(g_suki_win32.hwnd, &pos)) {
            input->mouse_position.x = (float)pos.x;
            input->mouse_position.y = (float)pos.y;
        }
    }
    // @Incomplete :: change mouse cursor only when needed
    // win32_update_mouse_cursor();
}

void win32_new_frame()
{
    Suki_Input *input;
    input = suki_get_input_state();
    // Setup display size (every frame to accommodate for window resizing)
    RECT rect;
    GetClientRect(g_suki_win32.hwnd, &rect);

    INT64 current_time;
    QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
    input->delta_time = (float)(current_time - g_suki_win32.time) / g_suki_win32.ticks_per_second;
    g_suki_win32.time = current_time;

    win32_update_mouse_position();

#if defined(SUKI_D3D11)
    d3d11_new_frame();
#endif

    // Read keyboard modifiers inputs
    input->key_ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    input->key_shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    input->key_alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    input->key_super = false;
}

int32_t win32_poll_events(void)
{
    if (PeekMessage(&g_suki_win32.msg, NULL, 0U, 0U, PM_REMOVE)) {
        TranslateMessage(&g_suki_win32.msg);
        DispatchMessage(&g_suki_win32.msg);
        return 0;
    }
    return 1;
}
