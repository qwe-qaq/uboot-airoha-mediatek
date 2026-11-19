/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2026 OpenWrt.org
 */

#ifndef _MEDIATEK_PANEL_LOGO_H
#define _MEDIATEK_PANEL_LOGO_H

#if IS_ENABLED(CONFIG_VIDEO_ST7789P3)
void board_show_logo(void);
void fdt_mark_panel_initialised(void *blob);
#else
static inline void board_show_logo(void) { }
static inline void fdt_mark_panel_initialised(void *blob) { }
#endif

#endif /* _MEDIATEK_PANEL_LOGO_H */
