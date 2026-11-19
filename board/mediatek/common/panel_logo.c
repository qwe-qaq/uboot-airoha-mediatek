// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2026 OpenWrt.org
 *
 * Draws the OpenWrt logo and the stage name on an ST7789P3 front panel. The
 * boards that carry one share the picture, the text position and the panel
 * geometry, so only the calls into here belong in a board file.
 */

#include <bootm.h>
#include <dm.h>
#include <image.h>
#include <mapmem.h>
#include <splash.h>
#include <version.h>
#include <video.h>
#include <video_console.h>
#include <linux/libfdt.h>

#include "panel_logo.h"

/* openwrt_logo.bmp, incbin'd by the .bmp.o rule in Makefile.lib-u-boot */
extern u8 __splash_openwrt_logo_begin[];

/*
 * Only claimed when the panel actually came up under us. A static property
 * would be a promise that U-Boot ran, which is false for netboot or recovery,
 * and the panel is write-only so the kernel cannot check for itself.
 */
void fdt_mark_panel_initialised(void *blob)
{
	struct udevice *vid;
	int node;

	vid = uclass_try_first_device(UCLASS_VIDEO);
	if (!vid || !device_active(vid))
		return;

	node = fdt_node_offset_by_compatible(blob, -1, "panel-mipi-dbi-spi");
	if (node < 0)
		return;

	if (fdt_setprop_empty(blob, node, "bootloader-initialized"))
		printf("cannot mark the panel as initialised\n");
}

/*
 * The first scanline clear of the centred 280x83 logo, rounded up to the 8x16
 * font cell. scripts/gen-boot-logos.py in the OpenWrt tree puts the BL2 text
 * band at the same y with the same glyphs, so changing CONFIG_VIDEO_FONT_* here
 * needs the BL2 blob regenerated too.
 */
#define PANEL_TEXT_Y		176

static void panel_text(struct udevice *vid, const char *text)
{
	struct video_priv *vid_priv = dev_get_uclass_priv(vid);
	struct vidconsole_priv *cons;
	struct udevice *con;
	int len = strlen(text);
	int max, x;

	if (uclass_first_device_err(UCLASS_VIDEO_CONSOLE, &con))
		return;

	cons = dev_get_uclass_priv(con);
	max = vid_priv->xsize / cons->x_charsize;
	x = len < max ? (vid_priv->xsize - len * cons->x_charsize) / 2 : 0;

	vidconsole_set_cursor_pos(con, x, PANEL_TEXT_Y);
	vidconsole_put_stringn(con, text, max);
}

/*
 * Order matters. video_sync() runs the driver blit before it looks at its own
 * rate limit, and CONFIG_CYCLIC is off here, so every call puts a whole 320x240
 * frame on the wire, which is 24 ms the eye can see. video_fill_part() and
 * vidconsole_put_stringn() only touch the framebuffer. video_bmp_display() ends
 * in video_sync(), so it goes last and its sync is the only blit of the frame.
 *
 * NO_FB_CLEAR keeps the BL2 picture on the panel through probe, so nothing has
 * painted the framebuffer and the background is laid down here.
 */
static void panel_draw(const char *text)
{
	struct video_priv *priv;
	struct udevice *vid;

	if (uclass_first_device_err(UCLASS_VIDEO, &vid))
		return;

	priv = dev_get_uclass_priv(vid);
	video_fill_part(vid, 0, 0, priv->xsize, priv->ysize, priv->colour_bg);

	panel_text(vid, text);
	video_bmp_display(vid, map_to_sysmem(__splash_openwrt_logo_begin),
			  BMP_ALIGN_CENTER, BMP_ALIGN_CENTER, true);
}

void board_show_logo(void)
{
	panel_draw(PLAIN_VERSION);
}

/* The panel keeps this picture until Linux hands over to whatever draws next */
void board_preboot_os(void)
{
	char *desc;

	if (!images.fit_hdr_os ||
	    fit_get_desc(images.fit_hdr_os, images.fit_noffset_os, &desc))
		desc = "Booting Linux";

	panel_draw(desc);
}
