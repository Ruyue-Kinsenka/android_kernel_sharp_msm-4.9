/* drivers/gpu/drm/msm/sharp/drm_fps_debugfs.h  (Display Driver)
 *
 * Copyright (C) 2018 SHARP CORPORATION
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef DRM_FPS_DEBUGFS_H
#define DRM_FPS_DEBUGFS_H

#include "../msm_drv.h"

#ifdef CONFIG_ANDROID_ENGINEERING
extern void drm_fps_create_debugfs(struct msm_drm_private *priv);
#else /* CONFIG_ANDROID_ENGINEERING */
static void drm_fps_create_debugfs(struct msm_drm_private *priv)
{
	return;
}
#endif /* CONFIG_ANDROID_ENGINEERING */

#endif /* DRM_FPS_DEBUGFS_H */
