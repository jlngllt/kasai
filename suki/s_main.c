#include "suki.h"
#include "..\kaizen\HandmadeMath.h" // TODO : to delete

extern int32_t win32_is_running(void);
extern int32_t win32_new_frame(void);
extern int32_t win32_poll_events(void);
extern void    win32_initialize(int32_t x, int32_t y, const char *title);
extern int32_t d3d_initialize(void);
extern void    d3d_draw_data(Suki_Draw_Data *draw_data);
extern void    d3d_render(void);

void suki_get_current_time(uint32_t *seconds, uint32_t *fractions);

typedef struct suki_graphic_mode {
    int32_t x_size;
    int32_t y_size;
    bool fullscreen;
} Suki_Graphic_Mode;

struct suki {
    Suki_Input input;
    Suki_Draw_Data draw_data;
    void (*action_func)(Suki_Input *input, void *user);
    void *action_func_data;
    Suki_Graphic_Mode graphic_mode;
    uint32_t time[2];
    uint32_t fps;
} g_suki;

void suki_set_action(void (*action_func)(Suki_Input *, void *), void *user_data)
{
    g_suki.action_func = action_func;
    g_suki.action_func_data = user_data;
}

void suki_reshape_view(int32_t x_size, int32_t y_size)
{
    g_suki.graphic_mode.x_size = x_size;
    g_suki.graphic_mode.y_size = y_size;
}

void suki_draw_data(Suki_Draw_Data *draw_data)
{
    d3d_draw_data(draw_data);
}

Suki_Input *suki_get_input_state(void)
{
    return &g_suki.input;
}

Suki_Draw_Data *suki_get_draw_data(void)
{
    return &g_suki.draw_data;
}

void suki_set_draw_data(Suki_Draw_Data *draw_data)
{
    g_suki.draw_data = *draw_data;
}

void suki_event_reset(Suki_Input *input)
{
    input->button_event_count = 0;
}

void suki_button_set(int32_t user_id, int id, bool state, long character)
{
    Suki_Input *input;
    input = &g_suki.input;
    if(input->button_event_count < 16) {
        input->button_event[input->button_event_count].user_id = user_id;
        input->button_event[input->button_event_count].state = state;
        input->button_event[input->button_event_count].button = id;
        input->button_event[input->button_event_count++].character = character;
    }
}

void suki_initialize(int argc, char **argv, Suki_Context_Type context_type, const char *title, int32_t width, int32_t height)
{
    SUKI_UNUSED(argc);
    SUKI_UNUSED(argv);
    SUKI_UNUSED(context_type);

    g_suki.input.num_frame = 0;
    suki_get_current_time(&g_suki.time[0], &g_suki.time[1]);

    win32_initialize(width, height, title);
    d3d_initialize();
}

static void suki_action(Suki_Action_Mode mode)
{
    Suki_Input *input = suki_get_input_state();
    input->mode = mode;
    if (g_suki.action_func != NULL)
        g_suki.action_func(&g_suki.input, g_suki.action_func_data);
}

#if defined _WIN32
void suki_get_current_time(uint32_t *seconds, uint32_t *fractions)
{
    static LARGE_INTEGER frequency;
    static bool init = FALSE;
    LARGE_INTEGER counter;
    if(!init)
    {
        init = TRUE;
        QueryPerformanceFrequency(&frequency);
    }
    QueryPerformanceCounter(&counter);
    if(seconds != NULL)
        *seconds = (uint32_t)(counter.QuadPart / frequency.QuadPart);
    if(fractions != NULL)
        *fractions = (uint32_t)((((ULONGLONG) 0xffffffffU) * (counter.QuadPart % frequency.QuadPart)) / frequency.QuadPart);
}
#else
#include <sys/time.h>
void suki_get_current_time(uint32_t *seconds, uint32_t *fractions)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if(seconds != NULL)
        *seconds = tv.tv_sec;
    if(fractions != NULL)
        *fractions = tv.tv_usec * 1E-6 * (double) (uint32_t)~0;
}
#endif

double suki_get_time_delta(void)
{
    return g_suki.input.delta_time;
}

uint32_t suki_get_fps(void)
{
    return g_suki.fps;
}

void suki_time_update(void)
{
    uint32_t seconds, fractions;
    suki_get_current_time(&seconds, &fractions);
    g_suki.input.delta_time = (double)seconds - (double)g_suki.time[0] + ((double)fractions - (double)g_suki.time[1]) / (double)(0xffffffff);
    g_suki.input.minute_time += g_suki.input.delta_time / 60.0;
    if(g_suki.input.minute_time >= 1.0)
        g_suki.input.minute_time -= 1.0;
    g_suki.time[0] = seconds;
    g_suki.time[1] = fractions;
}

void suki_fps_update()
{
    g_suki.fps = (uint32_t)(1.0 / g_suki.input.delta_time);
}

void suki_main_loop(void)
{
    while (win32_is_running()) {
        if (!win32_poll_events()) {
            continue;
        }
        suki_time_update();
        suki_action(SUKI_ACTION_EVENT);
        win32_new_frame();
        suki_action(SUKI_ACTION_DRAW);
        d3d_render();
        g_suki.input.num_frame++;
        suki_fps_update();
        suki_action(SUKI_ACTION_MAIN);
        suki_event_reset(&g_suki.input);
    }
}
