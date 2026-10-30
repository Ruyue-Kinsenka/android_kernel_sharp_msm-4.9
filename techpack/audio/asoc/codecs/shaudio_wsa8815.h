/* kernel/msm-4.9/techpack/audio/asoc/codecs/shaudio_wsa8815.h
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

#ifndef SHAUDIO_WSA8815_H
#define SHAUDIO_WSA8815_H

int register_wsa8815_sdm845_notifier(struct notifier_block *nb);
int unregister_wsa8815_sdm845_notifier(struct notifier_block *nb);

#endif /* SHAUDIO_WSA8815 */
