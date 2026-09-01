/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __CS35L41_PRIVATE_H__
#define __CS35L41_PRIVATE_H__

#include <linux/completion.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include "wm_adsp.h"

struct classh_cfg {
	bool classh_bst_override;
	bool classh_algo_enable;
	int classh_bst_max_limit;
	int classh_mem_depth;
	int classh_release_rate;
	int classh_headroom;
	int classh_wk_fet_delay;
	int classh_wk_fet_thld;
};

struct irq_cfg {
	bool is_present;
	bool irq_pol_inv;
	bool irq_out_en;
	int irq_src_sel;
};

struct cs35l41_platform_data {
	bool sclk_frc;
	bool lrclk_frc;
	bool right_channel;
	bool amp_gain_zc;
	bool ng_enable;
	int bst_ind;
	int bst_vctrl;
	int bst_ipk;
	int bst_cap;
	int temp_warn_thld;
	int ng_pcm_thld;
	int ng_delay;
	int dout_hiz;
	struct irq_cfg irq_config1;
	struct irq_cfg irq_config2;
	struct classh_cfg classh_config;
	int mnSpkType;
	struct device_node *spk_id_gpio_p;
};

struct cs35l41_private {
	struct wm_adsp dsp;
	struct snd_soc_codec *codec;
	struct cs35l41_platform_data pdata;
	struct device *dev;
	struct regmap *regmap;
	struct regulator_bulk_data supplies[2];
	int num_supplies;
	int irq;
	int clksrc;
	int extclk_freq;
	int extclk_cfg;
	int sclk;
	unsigned int cspl_cmd;
	bool dspa_mode;
	bool i2s_mode;
	bool swire_mode;
	bool halo_booted;
	bool bus_spi;
	int reset_gpio;
	struct completion global_pup_done;
	struct completion global_pdn_done;
	struct completion mbox_cmd;
};

int cs35l41_probe(struct cs35l41_private *cs35l41,
			struct cs35l41_platform_data *pdata);

#endif /* __CS35L41_PRIVATE_H__ */