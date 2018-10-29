struct vmctx {
	int	fd;
	uint32_t lowmem_limit;
	int	memflags;
	size_t	lowmem;
	size_t	highmem;
	char	*baseaddr;
	char	*name;
};

void *paddr_guest2host(struct vmctx *ctx, uintptr_t addr, size_t len);
int bhyve_pci_init(struct vmctx *ctx);
void bhyve_pci_cfgrw(struct vmctx *ctx, int in, int bnum, int snum,
    int fnum, int coff, int bytes, uint32_t *val);

