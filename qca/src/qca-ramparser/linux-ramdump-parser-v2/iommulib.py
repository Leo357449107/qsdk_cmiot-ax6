# Copyright (c) 2014,2019 The Linux Foundation. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 and
# only version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

import rb_tree
import linux_list as llist
from mmu import phys_to_virt


ARM_SMMU_DOMAIN = 0
WIN_SMMU_DOMAIN = 1
WIN_SMMU_AARCH64_DOMAIN = 2


class Domain(object):
    def __init__(self, pg_table, redirect, ctx_list, client_name,
                 domain_type=WIN_SMMU_DOMAIN, level=3, domain_num=-1):
        self.domain_num = domain_num
        self.pg_table = pg_table
        self.redirect = redirect
        self.ctx_list = ctx_list
        self.client_name = client_name
        self.level = level
        self.domain_type = domain_type

    def __repr__(self):
        return "#%d: %s" % (self.domain_num, self.client_name)


class IommuLib(object):
    def __init__(self, ramdump):
        self.ramdump = ramdump
        self.domain_list = []

        root = self.ramdump.read_word('domain_root')

        list_head_attachments = self.ramdump.read_word(
                                                    'iommu_debug_attachments')

        if list_head_attachments is not None:
            list_head_arm_addr = self.ramdump.read_structure_field(
                list_head_attachments, 'struct list_head', 'prev')
            list_walker = llist.ListWalker(
                self.ramdump, list_head_arm_addr,
                self.ramdump.field_offset('struct iommu_debug_attachment',
                                          'list'))
            list_walker.walk(list_head_attachments,
                             self._iommu_domain_find_default,
                             self.domain_list)

        if root is not None:
            rb_walker = rb_tree.RbTreeWalker(self.ramdump)
            rb_walker.walk(root, self._iommu_domain_func, self.domain_list)

    def _iommu_domain_find_default(self, node, domain_list):
        domain_ptr = self.ramdump.read_structure_field(
            node, 'struct iommu_debug_attachment', 'domain')

        if not domain_ptr:
            return

        if self.ramdump.field_offset('struct iommu_domain', 'priv') \
                is not None:
            priv_ptr = self.ramdump.read_structure_field(
                domain_ptr, 'struct iommu_domain', 'priv')

            if not priv_ptr:
                return
        else:
            priv_ptr = None

        arm_smmu_ops = self.ramdump.addr_lookup('arm_smmu_ops')

        ptr = self.ramdump.read_structure_field(
            node, 'struct iommu_debug_attachment', 'group')
        if ptr is not None:
            dev_list = ptr + self.ramdump.field_offset(
                'struct iommu_group', 'devices')
            dev = self.ramdump.read_structure_field(
                dev_list, 'struct list_head', 'next')
            if self.ramdump.kernel_version >= (4, 14):
                client_name = self.ramdump.read_structure_cstring(
                    dev, 'struct group_device', 'name')
            else:
                client_name = self.ramdump.read_structure_cstring(
                    dev, 'struct iommu_device', 'name')
        else:
            """Older kernel versions have the field 'dev'
           instead of 'iommu_group'.
            """
            ptr = self.ramdump.read_structure_field(
                node, 'struct iommu_debug_attachment', 'dev')
            kobj_ptr = ptr + self.ramdump.field_offset('struct device', 'kobj')
            client_name = self.ramdump.read_structure_cstring(
                kobj_ptr, 'struct kobject', 'name')

        iommu_domain_ops = self.ramdump.read_structure_field(
            domain_ptr, 'struct iommu_domain', 'ops')
        if iommu_domain_ops is None or iommu_domain_ops == 0:
            return

        if iommu_domain_ops == arm_smmu_ops:
            if priv_ptr is not None:
                arm_smmu_domain_ptr = priv_ptr
            else:
                arm_smmu_domain_ptr = self.ramdump.container_of(
                    domain_ptr, 'struct arm_smmu_domain', 'domain')

            pgtbl_ops_ptr = self.ramdump.read_structure_field(
                arm_smmu_domain_ptr, 'struct arm_smmu_domain', 'pgtbl_ops')
            if pgtbl_ops_ptr is None or pgtbl_ops_ptr == 0:
                return

            level = 0
            fn = self.ramdump.read_structure_field(pgtbl_ops_ptr,
                    'struct io_pgtable_ops', 'map')
            if fn == self.ramdump.addr_lookup('av8l_fast_map'):
                level = 3
            else:
                arm_lpae_io_pgtable_ptr = self.ramdump.container_of(
                    pgtbl_ops_ptr, 'struct arm_lpae_io_pgtable', 'iop.ops')

                level = self.ramdump.read_structure_field(
                    arm_lpae_io_pgtable_ptr, 'struct arm_lpae_io_pgtable',
                    'levels')

            pg_table = self.ramdump.read_structure_field(
                arm_smmu_domain_ptr, 'struct arm_smmu_domain',
                'pgtbl_cfg.arm_lpae_s1_cfg.ttbr[0]')

            pg_table = phys_to_virt(self.ramdump, pg_table)

            domain_create = Domain(pg_table, 0, [], client_name,
                                   ARM_SMMU_DOMAIN, level)
            domain_list.append(domain_create)

        else:
            priv_pt_offset = self.ramdump.field_offset('struct msm_iommu_priv',
                                                       'pt')
            pgtable_offset = self.ramdump.field_offset('struct msm_iommu_pt',
                                                       'fl_table')
            redirect_offset = self.ramdump.field_offset('struct msm_iommu_pt',
                                                        'redirect')

            if priv_pt_offset is not None:
                pg_table = self.ramdump.read_u64(
                    priv_ptr + priv_pt_offset + pgtable_offset)
                redirect = self.ramdump.read_u64(
                   priv_ptr + priv_pt_offset + redirect_offset)

            if (self.ramdump.is_config_defined('CONFIG_IOMMU_AARCH64')):
                domain_create = Domain(pg_table, redirect, [], client_name,
                                       WIN_SMMU_AARCH64_DOMAIN)
            else:
                domain_create = Domain(pg_table, redirect, [], client_name,
                                       WIN_SMMU_DOMAIN)

            domain_list.append(domain_create)

    def _iommu_list_func(self, node, ctx_list):
        ctx_drvdata_name_ptr = self.ramdump.read_word(
            node + self.ramdump.field_offset('struct msm_iommu_ctx_drvdata',
                                             'name'))
        ctxdrvdata_num_offset = self.ramdump.field_offset(
            'struct msm_iommu_ctx_drvdata', 'num')
        num = self.ramdump.read_u32(node + ctxdrvdata_num_offset)
        if ctx_drvdata_name_ptr != 0:
            name = self.ramdump.read_cstring(ctx_drvdata_name_ptr, 100)
            ctx_list.append((num, name))

    def _iommu_domain_func(self, node, domain_list):
        domain_num = self.ramdump.read_u32(self.ramdump.sibling_field_addr(
            node, 'struct msm_iova_data', 'node', 'domain_num'))
        domain = self.ramdump.read_word(self.ramdump.sibling_field_addr(
            node, 'struct msm_iova_data', 'node', 'domain'))
        priv_ptr = self.ramdump.read_word(
            domain + self.ramdump.field_offset('struct iommu_domain', 'priv'))

        client_name_offset = self.ramdump.field_offset(
            'struct msm_iommu_priv', 'client_name')

        if client_name_offset is not None:
            client_name_ptr = self.ramdump.read_word(
                priv_ptr + self.ramdump.field_offset(
                    'struct msm_iommu_priv', 'client_name'))
            if client_name_ptr != 0:
                client_name = self.ramdump.read_cstring(client_name_ptr, 100)
            else:
                client_name = '(null)'
        else:
            client_name = 'unknown'

        list_attached_offset = self.ramdump.field_offset(
                'struct msm_iommu_priv', 'list_attached')

        if list_attached_offset is not None:
            list_attached = self.ramdump.read_word(priv_ptr +
                                                   list_attached_offset)
        else:
            list_attached = None

        priv_pt_offset = self.ramdump.field_offset('struct msm_iommu_priv',
                                                   'pt')
        pgtable_offset = self.ramdump.field_offset('struct msm_iommu_pt',
                                                   'fl_table')
        redirect_offset = self.ramdump.field_offset('struct msm_iommu_pt',
                                                    'redirect')

        if priv_pt_offset is not None:
            pg_table = self.ramdump.read_word(
                priv_ptr + priv_pt_offset + pgtable_offset)
            redirect = self.ramdump.read_u32(
                priv_ptr + priv_pt_offset + redirect_offset)
        else:
            # On some builds we are unable to look up the offsets so hardcode
            # the offsets.
            pg_table = self.ramdump.read_word(priv_ptr + 0)
            redirect = self.ramdump.read_u32(priv_ptr +
                                             self.ramdump.sizeof('void *'))

            # Note: On some code bases we don't have this pg_table and redirect
            # in the priv structure (see msm_iommu_sec.c). It only contains
            # list_attached. If this is the case we can detect that by checking
            # whether pg_table == redirect (prev == next pointers of the
            # attached list).
            if pg_table == redirect:
                # This is a secure domain. We don't have access to the page
                # tables.
                pg_table = 0
                redirect = None

        ctx_list = []
        if list_attached is not None and list_attached != 0:
            list_walker = llist.ListWalker(
                self.ramdump, list_attached,
                self.ramdump.field_offset('struct msm_iommu_ctx_drvdata',
                                          'attached_elm'))
            list_walker.walk(list_attached, self._iommu_list_func, ctx_list)

            if (self.ramdump.is_config_defined('CONFIG_IOMMU_AARCH64')):
                domain_create = Domain(pg_table, redirect, ctx_list, client_name,
                                       WIN_SMMU_AARCH64_DOMAIN, domain_num=domain_num)
            else:
                domain_create = Domain(pg_table, redirect, ctx_list, client_name,
                                       WIN_SMMU_DOMAIN, domain_num=domain_num)

            domain_list.append(domain_create)
