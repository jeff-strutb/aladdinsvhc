/*
 * video_tables.h: video's own tables (the video session's file).
 */
#ifndef ALADDIN_VIDEO_TABLES_H
#define ALADDIN_VIDEO_TABLES_H

#include <stdint.h>

/* the window effects' quarter sine: 64 steps rising 0 to 15
   (video/window_effects.c's HalfSineBuild) */
uint8_t quarter_sine_step(unsigned step);   /* step < 64 */

#endif
