/* _NVRM_COPYRIGHT_BEGIN_
 *
 * Copyright 1999-2001 by NVIDIA Corporation.  All rights reserved.  All
 * information contained herein is proprietary and confidential to NVIDIA
 * Corporation.  Any use, reproduction, or disclosure without the written
 * permission of NVIDIA Corporation is prohibited.
 *
 * _NVRM_COPYRIGHT_END_
 */

RM_STATUS NV_API_CALL
nv_alias_pages(
    nv_state_t *nv,
    NvU32 page_cnt,
    NvU32 contiguous,
    NvU32 cache_type,
    NvU64 guest_id,
    NvU64 *pte_array,
    void **priv_data
)
{
    nv_alloc_t *at;
    nv_linux_state_t *nvl = NV_GET_NVL_FROM_NV_STATE(nv);
    NvU32 i=0;
    nv_pte_t *page_ptr = NULL;
    
    page_cnt = RM_PAGES_TO_OS_PAGES(page_cnt);
    at = nvos_create_alloc(nvl->dev, page_cnt);

    if (at == NULL)
    {
        return RM_ERR_NO_FREE_MEM;
    }

    at->flags = nv_alloc_init_flags(cache_type, 0, contiguous);
    at->flags |= NV_ALLOC_TYPE_GUEST;
                 
    at->nv = nv;
    at->order = nv_calc_order(at->num_pages * PAGE_SIZE);

    for (i=0; i < at->num_pages; ++i)
    {
        page_ptr = at->page_table[i];
        
        if (contiguous && i>0)
        {
            page_ptr->dma_addr = pte_array[0] + (i << PAGE_SHIFT);
        }
        else
        {
            page_ptr->dma_addr  = pte_array[i];
        }

        page_ptr->phys_addr = page_ptr->dma_addr; 

        /* aliased pages will be mapped on demand. */
        page_ptr->virt_addr = 0x0;
    }
    at->key_mapping = (void *)at->page_table[0]->dma_addr;
    at->guest_id    = guest_id;

    nvl_add_alloc(nvl, at);

    *priv_data = at;
    NV_ATOMIC_INC(at->usage_count);

    return RM_OK;
}

/* Zero out PTEs for a range of contiguous virtual addresses */
static int
nv_clear_ptes(unsigned long virt_addr, unsigned long size)
{
    unsigned long page_count;
    unsigned int i;

    page_count = (size >> PAGE_SHIFT) + ((size & ~PAGE_MASK) ? 1 : 0);

    for (i = 0; i < page_count; i++)
    {
        pgd_t *pgd = NULL;
        pmd_t *pmd = NULL;
        pte_t *pte = NULL;
        unsigned long address;

        address = (unsigned long)virt_addr + i * PAGE_SIZE;

        pgd = NV_PGD_OFFSET(address, 1, NULL);
        if (!NV_PGD_PRESENT(pgd))
            goto failed;

        pmd = NV_PMD_OFFSET(address, pgd);
        if (!NV_PMD_PRESENT(pmd))
            goto failed;

        pte = NV_PTE_OFFSET(address, pmd);
        if (!NV_PTE_PRESENT(pte))
            goto failed;

        pte_clear(NULL, 0, pte);
    }
    return 0;

failed:
    return -EFAULT;
}

/* Get a kernel mapping to system memory pages 
 * owned by guest.   
 */
static unsigned long
nv_map_guest_pages(nv_alloc_t *at,
                   NvU32 address,
                   NvU32 page_count,
                   NvU32 page_idx)
{
    unsigned long virt_addr;
    NvU32 i;

    NV_IOREMAP(virt_addr, address,
               page_count * PAGE_SIZE);

    if (virt_addr == 0)
    {
        nv_printf(NV_DBG_ERRORS,
                  "NVRM: nv_map_guest_pages - Could not allocate VA space for guest page mapping\n");
        return 0;
    }

    /* clear the PTEs so we can remap the VA space */
    if (nv_clear_ptes(virt_addr, page_count * PAGE_SIZE) < 0)
    {
        nv_printf(NV_DBG_ERRORS,
                  "NVRM: nv_map_guest_pages - Cannot clear PTEs for virtual address 0x%x, size %d\n",
                  virt_addr, page_count * PAGE_SIZE);
        goto failed;
    }
    
    for(i=0; i < page_count; ++i)
    {
        if (direct_kernel_remap_pfn_range(virt_addr + (i * PAGE_SIZE),
                                          (unsigned long)at->page_table[page_idx + i]->guest_pfn,
                                          PAGE_SIZE,
                                          PAGE_KERNEL, 
                                          at->guest_id) < 0)
        {
            nv_printf(NV_DBG_ERRORS,
                      "NVRM: nv_map_guest_pages - failed to map guest allocated page #%d PFN 0x%lx\n", 
                      i, (unsigned long)at->page_table[page_idx + i]->phys_addr);
            
            goto failed;
        }
    }
    
    return virt_addr;

failed:
    NV_IOUNMAP(virt_addr, 
               page_count * PAGE_SIZE);
    return 0;
}

RM_STATUS NV_API_CALL
nv_guest_pfn_list(nv_state_t  *nv,
                  unsigned int key,
                  unsigned int pfn_count,
                  unsigned int offset_index,
                  unsigned int *user_pfn_list)
{
    nv_alloc_t *at;
    NvU32 i;
    unsigned int *pfn_list = NULL;
    nv_linux_state_t *nvl = NV_GET_NVL_FROM_NV_STATE(nv);
    RM_STATUS status = RM_OK;
    unsigned long addr = (unsigned long) key;

    at = nvl_find_alloc(nvl, (addr << PAGE_SHIFT), 
                        NV_ALLOC_TYPE_GUEST);
    if (at == NULL)
    {
        nv_printf(NV_DBG_ERRORS,
                  "NVRM: nv_guest_pfn_list - unable to locate aliased memory reference.\n");
        status = RM_ERR_INVALID_ARGUMENT;
        goto done;
    }

    if ((pfn_count + offset_index) > at->num_pages)
    {
        nv_printf(NV_DBG_ERRORS,
                  "NVRM: nv_guest_pfn_list - Invalid PFN count %d.\n",
                  pfn_count);
        status = RM_ERR_INVALID_ARGUMENT;
        goto done;
    }
    
    NV_KMALLOC(pfn_list, 
               (pfn_count * sizeof(unsigned int)));
    if (pfn_list == NULL)
    {
        nv_printf(NV_DBG_ERRORS,
                  "NVRM: nv_guest_pfn_list - Unable to allocate memory.\n");
        status = RM_ERR_NO_FREE_MEM;
        goto done;
    }

    if (copy_from_user(pfn_list, user_pfn_list,  \
                       (pfn_count * sizeof(unsigned int))))
    {
        nv_printf(NV_DBG_ERRORS,
                  "NVRM: nv_guest_pfn_list - failed to copy PFN list!\n");
        status = RM_ERR_NO_FREE_MEM;
        goto done;
    }

    for(i=offset_index; i < at->num_pages; ++i)
    {
        at->page_table[i]->guest_pfn = pfn_list[i];
    }

done:
    if (pfn_list != NULL)
    {
	NV_KFREE(pfn_list, 
		 (pfn_count * sizeof(unsigned int)));
    }
    return status;
}
