// suki 00.00
// suki is a platform abstraction to easyly create Windows, Gl or D3D context, get/set input

#ifndef _SUKI_H_
#define _SUKI_H_

#include <stdint.h>
#include <stdbool.h>
#include <float.h> // FLT_MAX
#include <math.h>

#if defined(_MSC_VER)
    #pragma warning(push)
    #pragma warning(disable:4201)   /* nonstandard extension used: nameless struct/union */
    #pragma warning(disable:4115)   /**/
#endif

#if defined(_WIN32)
        #define WIN32_LEAN_AND_MEAN
        #include <windows.h>
        #include <tchar.h>
    #if defined(SUKI_D3D9)
        #include <d3d9.h>
    #elif defined(SUKI_D3D11)
        #ifndef D3D11_NO_HELPERS
            #define D3D11_NO_HELPERS
        #endif
        #ifndef CINTERFACE
            #define CINTERFACE
        #endif
        #ifndef COBJMACROS
            #define COBJMACROS
        #endif
        #include <dxgi.h>
        #include <d3d11.h>
        #include <d3dcompiler.h>
        #define DIRECTINPUT_VERSION 0x0800
    #endif
#endif

#define SUKI_UNUSED(x) ((void)(x))

typedef enum suki_action_mode {
    SUKI_ACTION_DRAW,
    SUKI_ACTION_EVENT,
    SUKI_ACTION_MAIN,
} Suki_Action_Mode;

typedef struct suki_button_event {
    int32_t button;
    int32_t character;
    bool state;
    int32_t user_id;
} Suki_Button_Event;

typedef struct suki_mouse {
    float position_x, position_y;
    float mouse_wheel_w;
    float mouse_wheel_h;
} Suki_Mouse;

typedef struct suki_input {
    bool key_ctrl;
    bool key_shift;
    bool key_alt;
    bool key_super;

    // @Incomplete : use Suki_Mouse structure
    bool mouse_down[3];
    float mouse_wheel_w;
    float mouse_wheel_h;
    struct {
        float x, y;
    } mouse_position;

    double delta_time;
    double minute_time;
    uint32_t num_frame;
    Suki_Button_Event button_event[16];
    int32_t button_event_count;
    Suki_Action_Mode mode;
} Suki_Input;

#define SUKI_RGBA(r,g,b,a) ((a << 24) | (b << 16) | (g << 8) | (r << 0))

typedef struct {
    float x, y, z, rhw;
    uint32_t color; // rgba packed
    float u, v;
} Suki_Vertex;

typedef unsigned short Suki_Index;

#define SUKI_NUM_VERTICES_MAX 64
#define SUKI_NUM_INDEXES_MAX 64

typedef struct {
    uint32_t color_background; // RGBA
    uint32_t num_vertices, num_indexes;
    Suki_Vertex vertices[SUKI_NUM_VERTICES_MAX];
    Suki_Index indexes[SUKI_NUM_INDEXES_MAX];
} Suki_Draw_Data;

typedef enum suki_context_type {
    SUKI_CT_OPENGL,
    SUKI_CT_OPENGLES2,
    SUKI_CT_VULKAN,
#if defined(_WIN32)
    SUKI_CT_D3D9,
    SUKI_CT_D3D10,
    SUKI_CT_D3D11,
    SUKI_CT_D3D12,
#endif
} Suki_Context_Type;

Suki_Input     *suki_get_input_state(void);
Suki_Draw_Data *suki_get_draw_data(void);
double          suki_get_time_delta(void);
uint32_t        suki_get_fps(void);
void            suki_set_draw_data(Suki_Draw_Data *draw_data);
void            suki_set_action(void (*action_func)(Suki_Input *, void *), void *);
void            suki_initialize(int argc, char **argv, Suki_Context_Type context_type, const char *title, int32_t width, int32_t height);
void            suki_main_loop(void);

#endif
