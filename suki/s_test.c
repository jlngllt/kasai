#include "suki.h"
#include <stdio.h>

void s_test_input_handler(Suki_Input *input, void *user_pointer)
{
    static float jj = 0.1f;
    Suki_Draw_Data draw_data;
    SUKI_UNUSED(user_pointer);
    int32_t ii;
    if (input->mode == SUKI_ACTION_MAIN) {
        for (ii = 0; ii < input->button_event_count; ++ii) {
            if (input->button_event[ii].button == -1) {
                printf("%c", input->button_event[ii].character);
            }
            fflush(stdout);
        }
    }
    double delta = suki_get_time_delta();
    // if (input->mouse_wheel_w > 0.0f)
    //     printf("Mouse Wheel w = %f\n", input->mouse_wheel_w);
    // if (input->mouse_wheel_h > 0.0f)
    //     printf("Mouse Wheel h = %f\n", input->mouse_wheel_h);
    for (ii = 0; ii < 3; ++ii) {
        if (input->mouse_down[ii] == true) {
            printf("click on %d\n", ii);
        }
    }
    if (input->mode == SUKI_ACTION_DRAW)
    {
        static float inc = 0.f;
#if 0
        float pos_0_x = 10.f;
        float pos_0_y = 10.f;
        float pos_0_z = -8.f;
        float size_x  = 100.f;
        float size_y  = 100.f;
        inc += 0.01f;
#else
        static float pos_0_x = -0.5f;
        float pos_0_y = 0.f;
        float pos_0_z = 0.f;
        float size_x  = 0.5f;
        float size_y  = 0.5f;
        float size_z  = 0.5f;
#endif

        unsigned int red =  SUKI_RGBA(255, 0, 0, 255);
        unsigned int blue =  SUKI_RGBA(0, 0, 255, 255);
        unsigned int green =  SUKI_RGBA(0, 255, 0, 255);
        unsigned int yellow  =  SUKI_RGBA(255, 255, 0, 255);
        unsigned int white  =  SUKI_RGBA(255, 255, 255, 255);
        unsigned int black  =  SUKI_RGBA(0, 0, 0, 255);
        unsigned int other1  =  SUKI_RGBA(129, 2, 18, 255);
        Suki_Vertex vertices[SUKI_NUM_VERTICES_MAX];
        Suki_Index indexes[SUKI_NUM_INDEXES_MAX];

        draw_data.num_vertices = 0;
        // 0
        vertices[draw_data.num_vertices].x = 0;
        vertices[draw_data.num_vertices].y = 0;
        vertices[draw_data.num_vertices].z = 0;
        vertices[draw_data.num_vertices].rhw = 1.0f;
        vertices[draw_data.num_vertices].color = white;
        vertices[draw_data.num_vertices].u = 1.0f;
        vertices[draw_data.num_vertices++].v = 1.0f;
        // 1
        vertices[draw_data.num_vertices].x = size_x;
        vertices[draw_data.num_vertices].y = 0;
        vertices[draw_data.num_vertices].z = 0;
        vertices[draw_data.num_vertices].rhw = 1.0f;
        vertices[draw_data.num_vertices].color = red;
        vertices[draw_data.num_vertices].u = 1.0f;
        vertices[draw_data.num_vertices++].v = 1.0f;
        // 2
        vertices[draw_data.num_vertices].x = 0;
        vertices[draw_data.num_vertices].y = size_y;
        vertices[draw_data.num_vertices].z = 0;
        vertices[draw_data.num_vertices].rhw = 1.0f;
        vertices[draw_data.num_vertices].color = green;
        vertices[draw_data.num_vertices].u = 1.0f;
        vertices[draw_data.num_vertices++].v = 1.0f;
        // 3
        vertices[draw_data.num_vertices].x = size_x;
        vertices[draw_data.num_vertices].y = size_y;
        vertices[draw_data.num_vertices].z = 0;
        vertices[draw_data.num_vertices].rhw = 1.0f;
        vertices[draw_data.num_vertices].color = blue;
        vertices[draw_data.num_vertices].u = 1.0f;
        vertices[draw_data.num_vertices++].v = 1.0f;
        // 4
        vertices[draw_data.num_vertices].x = 0;
        vertices[draw_data.num_vertices].y = 0;
        vertices[draw_data.num_vertices].z = size_z;
        vertices[draw_data.num_vertices].rhw = 1.0f;
        vertices[draw_data.num_vertices].color = white;
        vertices[draw_data.num_vertices].u = 1.0f;
        vertices[draw_data.num_vertices++].v = 1.0f;
        // 4
        vertices[draw_data.num_vertices].x = 0;
        vertices[draw_data.num_vertices].y = size_y;
        vertices[draw_data.num_vertices].z = size_z;
        vertices[draw_data.num_vertices].rhw = 1.0f;
        vertices[draw_data.num_vertices].color = black;
        vertices[draw_data.num_vertices].u = 1.0f;
        vertices[draw_data.num_vertices++].v = 1.0f;
        // 6
        vertices[draw_data.num_vertices].x = size_x;
        vertices[draw_data.num_vertices].y = size_y;
        vertices[draw_data.num_vertices].z = size_z;
        vertices[draw_data.num_vertices].rhw = 1.0f;
        vertices[draw_data.num_vertices].color = yellow;
        vertices[draw_data.num_vertices].u = 1.0f;
        vertices[draw_data.num_vertices++].v = 1.0f;
        // 7
        vertices[draw_data.num_vertices].x = size_x;
        vertices[draw_data.num_vertices].y = 0;
        vertices[draw_data.num_vertices].z = size_z;
        vertices[draw_data.num_vertices].rhw = 1.0f;
        vertices[draw_data.num_vertices].color = other1;
        vertices[draw_data.num_vertices].u = 1.0f;
        vertices[draw_data.num_vertices++].v = 1.0f;

        draw_data.num_indexes = 0;
        indexes[draw_data.num_indexes++] = 0;
        indexes[draw_data.num_indexes++] = 2;
        indexes[draw_data.num_indexes++] = 1;
        indexes[draw_data.num_indexes++] = 1;
        indexes[draw_data.num_indexes++] = 2;
        indexes[draw_data.num_indexes++] = 3;

        indexes[draw_data.num_indexes++] = 0;
        indexes[draw_data.num_indexes++] = 2;
        indexes[draw_data.num_indexes++] = 4;
        indexes[draw_data.num_indexes++] = 4;
        indexes[draw_data.num_indexes++] = 2;
        indexes[draw_data.num_indexes++] = 5;

        indexes[draw_data.num_indexes++] = 5;
        indexes[draw_data.num_indexes++] = 2;
        indexes[draw_data.num_indexes++] = 6;
        indexes[draw_data.num_indexes++] = 6;
        indexes[draw_data.num_indexes++] = 2;
        indexes[draw_data.num_indexes++] = 3;

        indexes[draw_data.num_indexes++] = 6;
        indexes[draw_data.num_indexes++] = 7;
        indexes[draw_data.num_indexes++] = 3;
        indexes[draw_data.num_indexes++] = 3;
        indexes[draw_data.num_indexes++] = 7;
        indexes[draw_data.num_indexes++] = 1;

        indexes[draw_data.num_indexes++] = 4;
        indexes[draw_data.num_indexes++] = 0;
        indexes[draw_data.num_indexes++] = 7;
        indexes[draw_data.num_indexes++] = 7;
        indexes[draw_data.num_indexes++] = 0;
        indexes[draw_data.num_indexes++] = 1;

        indexes[draw_data.num_indexes++] = 4;
        indexes[draw_data.num_indexes++] = 5;
        indexes[draw_data.num_indexes++] = 7;
        indexes[draw_data.num_indexes++] = 7;
        indexes[draw_data.num_indexes++] = 5;
        indexes[draw_data.num_indexes++] = 6;

        draw_data.color_background = SUKI_RGBA(125, 123, 124, 127);
        int32_t ii = 0;
        for (ii = 0; ii < draw_data.num_vertices; ++ii)
        {
            draw_data.vertices[ii] = vertices[ii];
        }
        for (ii = 0; ii < draw_data.num_indexes; ++ii)
        {
            draw_data.indexes[ii] = indexes[ii];
        }

        suki_set_draw_data(&draw_data);
    }
}

int main(int argc, char **argv)
{
    suki_initialize(argc, argv, SUKI_CT_D3D9, "Suki Test", 640, 480);
    suki_set_action(s_test_input_handler, NULL);
    suki_main_loop();
    return 0;
}
