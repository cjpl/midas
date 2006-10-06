/*
 * Name: pmcda816_module.c
 *
 * Kernel-side driver for the
 * ALPHI TECHNOLOGY CORPORATION
 * PMC-DA816
 * 8-channel, 16-bit dual-buffered digital-to-analog
 * signal generator.
 *
 * See manual at
 * http://www.alphitech.com
 * http://www.alphitech.com/doc/pmcda816.pdf
 *
 * $Id$
 *
 */

#include <linux/version.h>
#include <linux/config.h>

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/poll.h>

#if 0

/* Copy pgprot_noncached() from .../drivers/char/mem.c */

#ifndef pgprot_noncached

/*
 * This should probably be per-architecture in <asm/pgtable.h>
 */
static inline pgprot_t pgprot_noncached(pgprot_t _prot)
{
	unsigned long prot = pgprot_val(_prot);

#if defined(__i386__)
	/* On PPro and successors, PCD alone doesn't always mean 
	    uncached because of interactions with the MTRRs. PCD | PWT
	    means definitely uncached. */ 
	if (boot_cpu_data.x86 > 3)
		prot |= _PAGE_PCD | _PAGE_PWT;
#elif defined(__powerpc__)
	prot |= _PAGE_NO_CACHE | _PAGE_GUARDED;
#elif defined(__mc68000__)
#ifdef SUN3_PAGE_NOCACHE
	if (MMU_IS_SUN3)
		prot |= SUN3_PAGE_NOCACHE;
	else
#endif
	if (MMU_IS_851 || MMU_IS_030)
		prot |= _PAGE_NOCACHE030;
	/* Use no-cache mode, serialized */
	else if (MMU_IS_040 || MMU_IS_060)
		prot = (prot & _CACHEMASK040) | _PAGE_NOCACHE_S;
#elif defined(__mips__)
	prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;
#elif defined(__arm__) && defined(CONFIG_CPU_32)
	/* Turn off caching for all I/O areas */
	prot &= ~(L_PTE_CACHEABLE | L_PTE_BUFFERABLE);
#endif

	return __pgprot(prot);
}

#endif /* !pgprot_noncached */

#endif

static struct pci_dev* udev_pdev = NULL;
static u8    udev_irq = 0;
static char* udev_amcc_regs = NULL;
static char* udev_host_regs = NULL;
static char* udev_data_regs = NULL;

static int udev_host_pfn = 0;
static int udev_data_pfn = 0;

static int udev_intr_count = 0;

static int regs_open(struct inode * inode, struct file * file)
{
  //printk(KERN_INFO "pmcda816::regs_open: Here!\n");
  return 0;
}

static int regs_release(struct inode * inode, struct file * file)
{
  //printk(KERN_INFO "pmcda816::regs_release: Here!\n");
  //printk(KERN_INFO "pmcda816:: intr count: %d\n", udev_intr_count);
  //printk(KERN_INFO "pmcda816 AMCC S5933 register 0x%02x: 0x%08x\n", 0x3C, ioread32(udev_amcc_regs + 0x3C));
  return 0;
}

static int regs_mmap(struct file *file,struct vm_area_struct *vma)
{
  /* Only allow mmap() with these arguments */
  const u32 enforce_size_regs = 0x01000;
  const u32 enforce_size_data = 0x80000;

  /* mmap() arguments */
  u32 offset = vma->vm_pgoff << PAGE_SHIFT;
  u32 size   = vma->vm_end - vma->vm_start;

  if (0)
    {
      printk(KERN_INFO "pmcda816::regs_mmap: start: 0x%08lx, end: 0x%08lx, vm_pgoff: 0x%lx, offset: 0x%x, size: 0x%x\n",
	     vma->vm_start,
	     vma->vm_end,
	     vma->vm_pgoff,
	     offset,
	     size);
    }

  if (offset == 0)
    {
      int err;

      if (size != enforce_size_regs)
	{
	  printk(KERN_ERR "pmcda816::regs_mmap: Error: Attempt to mmap PMC-DA816 registers with map size 0x%x != 0x%x\n",size,enforce_size_regs);
	  return -EINVAL;
	}

      /* MMIO pages should be non-cached */
      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

      /* Don't try to swap out physical pages.. */
      vma->vm_flags |= VM_RESERVED;

      /* Don't dump addresses that are not real memory to a core file. */
      vma->vm_flags |= VM_IO;

      err = remap_pfn_range(vma, vma->vm_start, udev_host_pfn, vma->vm_end - vma->vm_start, vma->vm_page_prot);
      if (err)
	{
	  printk(KERN_ERR "pmcda816::regs_mmap: remap_pfn_range (regs) err %d\n", err);
	  return -EIO;
	}
    }
  else
    {
      int err;

      if (size != enforce_size_data)
	{
	  printk(KERN_ERR "pmcda816::regs_mmap: Error: Attempt to mmap PMC-DA816 data buffer with map size 0x%x != 0x%x\n",size,enforce_size_data);
	  return -EINVAL;
	}

      /* MMIO pages should be non-cached */
      vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

      /* Don't try to swap out physical pages.. */
      vma->vm_flags |= VM_RESERVED;

      /* Don't dump addresses that are not real memory to a core file. */
      vma->vm_flags |= VM_IO;

      err = remap_pfn_range(vma, vma->vm_start, udev_data_pfn, size, vma->vm_page_prot);
      if (err)
	{
	  printk(KERN_ERR "pmcda816::regs_mmap: remap_pfn_range (data) at 0x%lx, data pfn 0x%x, size %d, err %d\n", vma->vm_start, udev_data_pfn, size, err);
	  return -EIO;
	}
    }

  return 0;
}

static struct file_operations regs_fops = {
  mmap:		regs_mmap,
  open:		regs_open,
  release:	regs_release
};

static int intr_handler(int irq, void *dev_id, struct pt_regs *regs)
{
  int mine;
  u32 stat;

  stat = ioread8(udev_host_regs + 0x10);
  mine = stat & 0xF0000000;
  if (mine)
    {
      iowrite8(0xF0, udev_host_regs + 0x13);
      printk(KERN_INFO "pmcda816: interrupt irq %d, stat 0x%08x, count %d\n", irq, stat, udev_intr_count++);
      return IRQ_HANDLED;
    }

  printk(KERN_INFO "pmcda816: (for somebody else...) interrupt irq %d, stat 0x%08x, count %d\n", irq, stat, udev_intr_count++);
  return IRQ_NONE;
}

static int init(struct pci_dev *pdev)
{
  printk(KERN_INFO "pmcda816: Device %s at 0x%p\n", pci_name(pdev), pdev);

  udev_pdev = pdev;

  /* Enable PCI device */
  pci_enable_device(pdev);

  /* Enable PCI bus-master */
  pci_set_master(pdev);

  /* Setup BAR0: AMCC 5933 PCI Operation Registers */

  if (! (pci_resource_flags(pdev,0) & IORESOURCE_MEM))
    {
      printk(KERN_ERR "pmcda816: Error: BAR0 is not MMIO\n");
      return -ENODEV;
    }

  if (!request_mem_region(pci_resource_start(pdev,0), pci_resource_len(pdev,0), "pmcda816-amcc"))
    {
      printk (KERN_ERR "pmcda816: Error: Cannot allocate the PCI MMIO BAR0 region for AMCC 5933 PMC Operation Registers at 0x%lx, size 0x%lx\n",
	      pci_resource_start (pdev, 0),
	      pci_resource_len (pdev, 0));
      return -ENODEV;
    }

  udev_amcc_regs = ioremap_nocache(pci_resource_start(pdev,0), pci_resource_len(pdev,0));
  printk(KERN_INFO "pmcda816: PCI BAR0 mapped to local addr 0x%p\n", udev_amcc_regs);

  if (0)
    {
      int i;
      for (i=0x3C; i<=0x3C; i+=4)
	{
	  int w = ioread32(udev_amcc_regs + i);
	  printk(KERN_INFO "pmcda816: AMCC S5933 register 0x%02x: 0x%08x\n", i, w);
	}
      
      //iowrite32(0x0000FFFF, udev_amcc_regs + 0x3C);
    }

  /* Setup BAR1 : Host control region */

  if (! (pci_resource_flags(pdev,1) & IORESOURCE_MEM))
    {
      printk(KERN_ERR "pmcda816: Error: BAR1 is not MMIO\n");
      return -ENODEV;
    }

  if (!request_mem_region(pci_resource_start(pdev,1), pci_resource_len(pdev,1), "pmcda816-control"))
    {
      printk (KERN_ERR "pmcda816: Error: Cannot allocate the PCI MMIO BAR1 region for Host Control Region at 0x%lx, size 0x%lx\n",
	      pci_resource_start (pdev, 1),
	      pci_resource_len (pdev, 1));
      return -ENODEV;
    }

  udev_host_pfn = pci_resource_start(pdev,1) >> PAGE_SHIFT;
  udev_host_regs = ioremap_nocache(pci_resource_start(pdev,1), pci_resource_len(pdev,1));
  printk(KERN_INFO "pmcda816: PCI BAR1 mapped to local addr 0x%p\n", udev_host_regs);

  if (1)
    {
      int i;
      for (i=0; i<0x14; i+=4)
	{
	  int w = ioread32(udev_host_regs + i);
	  printk(KERN_INFO "pmcda816: register 0x%02x: 0x%08x\n", i, w);
	}
    }

  if (0)
    {
      /* reset the device registers 
       * stop the DACs
       * enable access to the data memory */

      int i;
      for (i=0; i<0x14; i+=4)
	iowrite32(0, udev_host_regs + i);
    }

  /* Setup BAR3 : RAM buffer region */

  if (! (pci_resource_flags(pdev,3) & IORESOURCE_MEM))
    {
      printk(KERN_ERR "pmcda816: Error: BAR3 is not MMIO\n");
      return -ENODEV;
    }

  if (!request_mem_region(pci_resource_start(pdev,3), pci_resource_len(pdev,3), "pmcda816-data"))
    {
      printk (KERN_ERR "pmcda816: Error: Cannot allocate the PCI MMIO BAR3 region for RAM Data Buffer Region at 0x%lx, size 0x%lx\n",
	      pci_resource_start (pdev, 3),
	      pci_resource_len (pdev, 3));
      return -ENODEV;
    }

  udev_data_pfn = pci_resource_start(pdev,3) >> PAGE_SHIFT;
  udev_data_regs = ioremap_nocache(pci_resource_start(pdev,3), pci_resource_len(pdev,3));
  printk(KERN_INFO "pmcda816: PCI BAR3 mapped to local addr 0x%p\n", udev_data_regs);

#if 0
  {
    int i;
    for (i=0; i<0x100; i+=4)
      {
	int w = ioread32(udev_data_regs + i);
	printk(KERN_INFO "pmcda816: data 0x%06x: %08x\n", i, w);
      }

    for (i=0; i<0x80; i+=4)
      {
	iowrite32(i, udev_data_regs + i);
      }

    for (i=0; i<0x100; i+=4)
      {
	int w = ioread32(udev_data_regs + i);
	printk(KERN_INFO "pmcda816: data 0x%06x: %08x\n", i, w);
      }
  }
#endif

  udev_irq = pdev->irq;
  printk(KERN_INFO "pmcda816: Using irq %d\n", udev_irq);

  /* Connect interrupts */
  if (udev_irq > 0)
    {
      int err;
      /* we may use shared interrupts */
      err = request_irq(udev_irq, intr_handler, SA_SHIRQ, "pmcda816", udev_pdev);
      if (err)
	{
	  printk(KERN_ERR "pmcda816: Cannot get irq %d, err %d\n", udev_irq, err);
	  udev_irq = 0;
	}

      //enable_irq(udev_irq);
    }

  if (register_chrdev(MOD_MAJOR, MOD_NAME, &regs_fops)) {
    printk(KERN_ERR "pmcda816: Cannot register device\n");
    return -1;
  }

  return 0;
}

void cleanup_module()
{
  printk(KERN_INFO "pmcda816: cleanup_module\n");

  //disable_intr();

  if (udev_irq > 0)
    {
      free_irq(udev_irq, udev_pdev);
    }

  unregister_chrdev(MOD_MAJOR, MOD_NAME);

  if (udev_amcc_regs)
    iounmap(udev_amcc_regs);

  if (udev_host_regs)
    iounmap(udev_host_regs);

  if (udev_data_regs)
    iounmap(udev_data_regs);

  release_mem_region(pci_resource_start(udev_pdev,0), pci_resource_len(udev_pdev,0));
  release_mem_region(pci_resource_start(udev_pdev,1), pci_resource_len(udev_pdev,1));
  release_mem_region(pci_resource_start(udev_pdev,3), pci_resource_len(udev_pdev,3));

  printk(KERN_INFO "pmcda816: driver removed\n");
}

/*
 * the initialisation routine.
 * Return: 0 if okey, -Exxx if error.
 */
int init_module(void)
{
  printk(KERN_INFO "pmcda816: Linux driver by K.Olchanski/TRIUMF\n");
  printk(KERN_INFO "pmcda816: driver compiled " __DATE__ " " __TIME__ "\n");

  {
    int n=0;
    struct pci_dev *pdev = NULL;
    while ((pdev = pci_find_device(0x13c5, 0x010a, pdev)))
      {
	int err = 0;

	printk(KERN_INFO "pmcda816: Found device %p\n", pdev);

	n++;
	if (n > 1)
	  {
	    printk(KERN_ERR "pmcda816: Cannot handle more than one device, please contact author");
	    return -ENOSYS;
	  }

	err = init(pdev);
	if (err)
	  return err;
      }

    if (n == 0)
      {
	printk(KERN_ERR "pmcda816: No PMC-DA816 devices found.\n");
	return -ENOSYS;
      }
  }

  return 0;
}

/* end file */
