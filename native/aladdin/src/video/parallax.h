/*
 * parallax.h: the backdrop's scroll from the play field's (video/parallax.c,
 * backdrop_offset.c). ParallaxUpdate is in video_internal.h.
 */
#ifndef ALADDIN_PARALLAX_H
#define ALADDIN_PARALLAX_H

void ParallaxUpdate(void);                  /* 81:C772 */
void BackdropFollowOffsetY(void);           /* 81:CA6A */

/* wanted (map/): 81:B756 PlayerOnFirmGround, 1 when the player stands on
   firm ground at probe 0 (the original's C set) */
int PlayerOnFirmGround(void);

#endif
