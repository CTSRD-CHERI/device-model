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
