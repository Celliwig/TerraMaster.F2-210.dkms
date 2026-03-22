// SPDX-License-Identifier: GPL-2.0
/* Taken from drivers/clk/clk.c */
#ifndef __LINUX_CLK_CORE_H
#define __LINUX_CLK_CORE_H

struct clk_parent_map {
        const struct clk_hw     *hw;
        struct clk_core         *core;
        const char              *fw_name;
        const char              *name;
        int                     index;
};

struct clk_core {
        const char              *name;
        const struct clk_ops    *ops;
        struct clk_hw           *hw;
        struct module           *owner;
        struct device           *dev;
        struct hlist_node       rpm_node;
        struct device_node      *of_node;
        struct clk_core         *parent;
        struct clk_parent_map   *parents;
        u8                      num_parents;
        u8                      new_parent_index;
        unsigned long           rate;
        unsigned long           req_rate;
        unsigned long           new_rate;
        struct clk_core         *new_parent;
        struct clk_core         *new_child;
        unsigned long           flags;
        bool                    orphan;
        bool                    rpm_enabled;
        unsigned int            enable_count;
        unsigned int            prepare_count;
        unsigned int            protect_count;
        unsigned long           min_rate;
        unsigned long           max_rate;
        unsigned long           accuracy;
        int                     phase;
        struct clk_duty         duty;
        struct hlist_head       children;
        struct hlist_node       child_node;
        struct hlist_head       clks;
        unsigned int            notifier_count;
#ifdef CONFIG_DEBUG_FS
        struct dentry           *dentry;
        struct hlist_node       debug_node;
#endif
        struct kref             ref;
};

struct clk {
        struct clk_core *core;
        struct device *dev;
        const char *dev_id;
        const char *con_id;
        unsigned long min_rate;
        unsigned long max_rate;
        unsigned int exclusive_count;
        struct hlist_node clks_node;
};

#endif /* __LINUX_CLK_CORE_H */
