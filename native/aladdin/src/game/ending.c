/*
 * ending.c: the ending: its zoom scene's task, the stages task that shows
 * the levels played back, and the text pages with their extras.
 */
#include "game/game.h"
#include "system/state.h"
#include "system/task.h"
#include "video/video.h"

#define CENTRE_X 0x80
#define CENTRE_Y 0x50
#define SPEED_FIRST 4
#define ZOOM_FIRST 8
#define SPEED_EVERY 16              /* frames between speed-ups, and zoom steps */
#define ZOOM_EVERY 0x18
#define SCALED_EACH 1               /* g.mode7_how: each matrix element scaled first */

enum { SPIN_START, SPIN_WAITED };

/*
 * @routine 80:B1D8 SceneSpinZoomTask
 * WHAT IT DOES: Spins and zooms a Mode 7 scene: the angle steps on by a
 * speed that grows every 16 frames and the zoom grows every 24, the zoom
 * taken as its negative past $7F.
 * HOW: The centre ($80, $50); Mode 7 scaled (g.mode7_how 1); angle 0 (the
 * task's parameter), speed 4, zoom 8, counts 16 and 16. Each frame: the
 * counts step (speed up every 16, zoom up every 24); the zoom (negated when
 * negative) into the four scales; the matrix words a and b shifted left 4,
 * d = a and c = -b; a frame's wait; Mode7Rotate by the angle, then the
 * angle plus the speed. Never ends.
 */
TaskRun SceneSpinZoomTask(Task *t)
{
    uint16_t *at = task_enter(t);
    uint8_t zoom;

    if (*at == SPIN_START) {
        g.m7_centre_x = CENTRE_X;
        g.m7_centre_y = CENTRE_Y;
        g.mode7_how = SCALED_EACH;
        t->param = 0;
        t->own.spin_zoom.speed = SPEED_FIRST;
        t->own.spin_zoom.zoom = ZOOM_FIRST;
        t->own.spin_zoom.speed_count = SPEED_EVERY;
        t->own.spin_zoom.zoom_count = SPEED_EVERY;
    } else {
        Mode7Rotate(t->param);
        t->param = (uint8_t)(t->param + t->own.spin_zoom.speed);
    }
    if (--t->own.spin_zoom.speed_count == 0) {
        t->own.spin_zoom.speed_count = SPEED_EVERY;
        t->own.spin_zoom.speed++;
    }
    if (--t->own.spin_zoom.zoom_count == 0) {
        t->own.spin_zoom.zoom_count = ZOOM_EVERY;
        t->own.spin_zoom.zoom++;
    }
    zoom = (uint8_t)t->own.spin_zoom.zoom;
    if (zoom & 0x80)
        zoom = (uint8_t)(~zoom + 1);
    for (int i = 0; i < 4; i++)
        g.mode7_scale[i] = zoom;
    g.m7_matrix[0] = (int16_t)((uint16_t)g.m7_matrix[0] << 4);
    g.m7_matrix[3] = g.m7_matrix[0];
    g.m7_matrix[1] = (int16_t)((uint16_t)g.m7_matrix[1] << 4);
    g.m7_matrix[2] = (int16_t)(0 - (uint16_t)g.m7_matrix[1]);
    *at = SPIN_WAITED;
    return task_wait(t, 1);
}
