/* drivers/gpu/drm/msm/sharp/drm_voltage_control.h  (Display Driver)
 *
 * Copyright (C) 2019 SHARP CORPORATION
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

#ifndef DRM_VOLTAGE_CONTROL_H
#define DRM_VOLTAGE_CONTROL_H
extern void drm_voltage_control_init(struct device *dev);
extern void drm_voltage_control_panel_enable(void);
extern void drm_voltage_control_panel_disable(void);
#endif /* DRM_VOLTAGE_CONTROL_H */
