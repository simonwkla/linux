// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
#include <linux/dma-mapping.h>
#include "mlx5_ib.h"

struct mlx5_user_mmap_entry *mlx5_ib_alloc_coh_buf(
	struct mlx5_ib_dev *dev,
	struct mlx5_ib_ucontext *ctx,
	size_t size
) {
	struct mlx5_user_mmap_entry *m;
	int err;

	m = kzalloc(sizeof(*m), GFP_KERNEL);
	if(!m){
		return ERR_PTR(-ENOMEM);
	}

	m->coh_size = PAGE_ALIGN(size);
	m->coh_vaddr = dma_alloc_coherent(
		&dev->mdev->pdev->dev,
		m->coh_size,
		&m->coh_dma,
		GFP_KERNEL | __GFP_ZERO 
	);
	if(!m->coh_vaddr){
		mlx5_ib_warn(dev, "coh_buf: dma_alloc_coherent failed size=%zu\n", m->coh_size);
		err = -ENOMEM;
		goto err_free_m;
	}

	m->mmap_flag = MLX5_IB_MMAP_TYPE_COH_BUF;

	err = mlx5_rdma_user_mmap_entry_insert(ctx, m, m->coh_size);
	if(err){
		mlx5_ib_warn(
			dev,
			"coh_buf: rdma_user_mmap_entry_insert_range failed err=%d size=%zu pgoff=[0x%x,0x%lx]\n",
			err, m->coh_size, MLX5_IB_MMAP_OFFSET_START << 16,
			((MLX5_IB_MMAP_OFFSET_END << 16) + (1UL << 16) - 1)
		);
		goto err_free_dma;
	}

	return m;

err_free_dma:
	dma_free_coherent(
		&dev->mdev->pdev->dev,
		m->coh_size,
		m->coh_vaddr,
		m->coh_dma
	);
err_free_m:
	kfree(m);
	return ERR_PTR(err);
}

void mlx5_ib_release_coh_buf(struct mlx5_user_mmap_entry *m) {
	if(!m) return;
	rdma_user_mmap_entry_remove(&m->rdma_entry);
}

