/*
 * Name: pmcsoftdacm_module.c
 *
 * Kernel-side driver for the
 * ALPHI TECHNOLOGY CORPORATION
 * PMC-SOFTDAC-M
 * 8-channel, 16-bit dual-buffered digital-to-analog
 * signal generator.
 *
 * See manual at
 * http://www.alphitech.com
 * http://www.alphitech.com/doc/???.pdf
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
static char* udev_plx9080_regs = NULL;
static char* udev_softdac_regs = NULL;

static int udev_plx9080_pfn = 0;
static int udev_softdac_pfn = 0;

static int udev_intr_count = 0;

static void plx9080_intr_show(void)
{
  u32 s =  ioread32(udev_plx9080_regs + 0x68);
  printk(KERN_INFO "pmcsoftdacm::plx9080_intr_show: INTCSR: 0x%08x\n", s);
}

static int plx9080_intr_enable(void)
{
  iowrite32(0x00000F00, udev_plx9080_regs + 0x68);
  // issue a PCI read to flush posted PCI writes
  return ioread32(udev_plx9080_regs + 0x68);
}

static int plx9080_intr_disable(void)
{
  iowrite32(0x00000000, udev_plx9080_regs + 0x68);
  // issue a PCI read to flush posted PCI writes
  return ioread32(udev_plx9080_regs + 0x68);
}

static int regs_open(struct inode * inode, struct file * file)
{
  //printk(KERN_INFO "pmcsoftdacm::regs_open: Here!\n");
  return 0;
}

static int regs_release(struct inode * inode, struct file * file)
{
  if (1)
    {
      //plx9080_intr_show();

      //int i;
      //for (i=0x68; i<=0x68; i+=4)
      //	{
      //	  int w = ioread32(udev_plx9080_regs + i);
      //	  printk(KERN_INFO "pmcsoftdacm::regs_release: PLX9080 register 0x%02x: 0x%08x\n", i, w);
      //	}
    }

  //printk(KERN_INFO "pmcsoftdacm::regs_release: Here!\n");
  //printk(KERN_INFO "pmcsoftdacm:: intr count: %d\n", udev_intr_count);
  //printk(KERN_INFO "pmcsoftdacm AMCC S5933 register 0x%02x: 0x%08x\n", 0x3C, ioread32(udev_amcc_regs + 0x3C));

  plx9080_intr_disable();

  return 0;
}

static int regs_mmap(struct file *file,struct vm_area_struct *vma)
{
  int err;

  /* Only allow mmap() with these arguments */
  const u32 enforce_size = 0x100000;

  /* mmap() arguments */
  u32 offset = vma->vm_pgoff << PAGE_SHIFT;
  u32 size   = vma->vm_end - vma->vm_start;

  if (0)
    {
      printk(KERN_INFO "pmcsoftdacm::regs_mmap: start: 0x%08lx, end: 0x%08lx, vm_pgoff: 0x%lx, offset: 0x%x, size: 0x%x\n",
	     vma->vm_start,
	     vma->vm_end,
	     vma->vm_pgoff,
	     offset,
	     size);
    }

  if (size != enforce_size)
    {
      printk(KERN_ERR "pmcsoftdacm::regs_mmap: Error: Attempt to mmap PMC-SOFTDAC-M registers with map size 0x%x != 0x%x\n",size,enforce_size);
      return -EINVAL;
    }

  /* MMIO pages should be non-cached */
  vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

  /* Don't try to swap out physical pages.. */
  vma->vm_flags |= VM_RESERVED;

  /* Don't dump addresses that are not real memory to a core file. */
  vma->vm_flags |= VM_IO;

  err = remap_pfn_range(vma, vma->vm_start, udev_softdac_pfn, vma->vm_end - vma->vm_start, vma->vm_page_prot);
  if (err)
    {
      printk(KERN_ERR "pmcsoftdacm::regs_mmap: remap_pfn_range (regs) err %d\n", err);
      return -EIO;
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
  u32 stat, statr;

  stat = ioread32(udev_softdac_regs + 0x80010);
  mine = stat & 0x70000000;
  if (mine)
    {
      //plx9080_intr_show();
      iowrite8(0x70, udev_softdac_regs + 0x80013);
      // issue a PCI read to flush posted PCI writes
      statr = ioread32(udev_softdac_regs + 0x80010);
      printk(KERN_INFO "pmcsoftdacm: interrupt irq %d, stat 0x%08x -> 0x%08x, count %d\n", irq, stat, statr, udev_intr_count++);
      //plx9080_intr_show();
      return IRQ_HANDLED;
    }

  printk(KERN_INFO "pmcsoftdacm: (for somebody else...) interrupt irq %d, stat 0x%08x, count %d\n", irq, stat, udev_intr_count++);

  return IRQ_NONE;
}

static int init(struct pci_dev *pdev)
{
  printk(KERN_INFO "pmcsoftdacm: Device %s at 0x%p\n", pci_name(pdev), pdev);

  udev_pdev = pdev;

  /* Enable PCI device */
  pci_enable_device(pdev);

  /* Enable PCI bus-master */
  pci_set_master(pdev);

  /* Setup BAR0: PLX9080 Operation Registers */

  if (! (pci_resource_flags(pdev,0) & IORESOURCE_MEM))
    {
      printk(KERN_ERR "pmcsoftdacm: Error: BAR0 is not MMIO\n");
      return -ENODEV;
    }

  if (!request_mem_region(pci_resource_start(pdev,0), pci_resource_len(pdev,0), "pmcsoftdacm-plx9080"))
    {
      printk (KERN_ERR "pmcsoftdacm: Error: Cannot allocate the PCI MMIO BAR0 region for PLX9080 Operation Registers at 0x%lx, length 0x%lx\n",
	      pci_resource_start (pdev, 0),
	      pci_resource_len (pdev, 0));
      return -ENODEV;
    }

  udev_plx9080_regs = ioremap_nocache(pci_resource_start(pdev,0), pci_resource_len(pdev,0));
  printk(KERN_INFO "pmcsoftdacm: PCI BAR0 mapped to local addr 0x%p, length 0x%lx\n", udev_plx9080_regs, pci_resource_len(pdev,0));

  if (1)
    {
      int i;
      for (i=0x68; i<=0x68; i+=4)
	{
	  int w = ioread32(udev_plx9080_regs + i);
	  printk(KERN_INFO "pmcsoftdacm: PLX9080 register 0x%02x: 0x%08x\n", i, w);
	}
    }

  /* Setup BAR2 : softdac memory and control region */

  if (! (pci_resource_flags(pdev,2) & IORESOURCE_MEM))
    {
      printk(KERN_ERR "pmcsoftdacm: Error: BAR2 is not MMIO\n");
      return -ENODEV;
    }

  if (!request_mem_region(pci_resource_start(pdev,2), pci_resource_len(pdev,2), "pmcsoftdacm-data"))
    {
      printk (KERN_ERR "pmcsoftdacm: Error: Cannot allocate the PCI MMIO BAR2 region for Data Buffer Region at 0x%lx, length 0x%lx\n",
	      pci_resource_start (pdev, 2),
	      pci_resource_len (pdev, 2));
      return -ENODEV;
    }

  udev_softdac_pfn = pci_resource_start(pdev,2) >> PAGE_SHIFT;
  udev_softdac_regs = ioremap_nocache(pci_resource_start(pdev,2), pci_resource_len(pdev,2));
  printk(KERN_INFO "pmcsoftdacm: PCI BAR2 mapped to local addr 0x%p, length 0x%lx\n", udev_softdac_regs, pci_resource_len(pdev,2));

#if 0
  {
    int i;
    for (i=0; i<0x100; i+=4)
      {
	int w = ioread32(udev_data_regs + i);
	printk(KERN_INFO "pmcsoftdacm: data 0x%06x: %08x\n", i, w);
      }

    for (i=0; i<0x80; i+=4)
      {
	iowrite32(i, udev_data_regs + i);
      }

    for (i=0; i<0x100; i+=4)
      {
	int w = ioread32(udev_data_regs + i);
	printk(KERN_INFO "pmcsoftdacm: data 0x%06x: %08x\n", i, w);
      }
  }
#endif

  udev_irq = pdev->irq;
  printk(KERN_INFO "pmcsoftdacm: Using irq %d\n", udev_irq);

  /* Connect interrupts */
  if (udev_irq > 0)
    {
      int err;
      /* we may use shared interrupts */
      err = request_irq(udev_irq, intr_handler, SA_INTERRUPT|SA_SHIRQ, "pmcsoftdacm", udev_pdev);
      if (err)
	{
	  printk(KERN_ERR "pmcsoftdacm: Cannot get irq %d, err %d\n", udev_irq, err);
	  udev_irq = 0;
	}

      enable_irq(udev_irq);

      // This enables the PMC-SOFTDAC-M interrupts -
      // commented out because this driver does not
      // do anything with them - we should only enable
      // interrupts when a user program is ready to do
      // something with them. We should call plx9080_intr_enable()
      // in the open() call, or in the mmap() call. or in a
      // "wait for interrupt" call.
      //plx9080_intr_enable();
    }

  if (register_chrdev(MOD_MAJOR, MOD_NAME, &regs_fops)) {
    printk(KERN_ERR "pmcsoftdacm: Cannot register device\n");
    return -1;
  }

  return 0;
}

void cleanup_module()
{
  printk(KERN_INFO "pmcsoftdacm: cleanup_module\n");

  plx9080_intr_disable();

  if (udev_irq > 0)
    {
      disable_irq(udev_irq);
      free_irq(udev_irq, udev_pdev);
    }

  unregister_chrdev(MOD_MAJOR, MOD_NAME);

  if (udev_plx9080_regs)
    iounmap(udev_plx9080_regs);

  if (udev_softdac_regs)
    iounmap(udev_softdac_regs);

  release_mem_region(pci_resource_start(udev_pdev,0), pci_resource_len(udev_pdev,0));
  release_mem_region(pci_resource_start(udev_pdev,2), pci_resource_len(udev_pdev,2));

  printk(KERN_INFO "pmcsoftdacm: driver removed\n");
}

/*
 * the initialisation routine.
 * Return: 0 if okey, -Exxx if error.
 */
int init_module(void)
{
  printk(KERN_INFO "pmcsoftdacm: Linux driver by K.Olchanski/TRIUMF\n");
  printk(KERN_INFO "pmcsoftdacm: driver compiled " __DATE__ " " __TIME__ "\n");

  {
    int n=0;
    struct pci_dev *pdev = NULL;
    while ((pdev = pci_find_device(0x13c5, 0x0313, pdev)))
      {
	int err = 0;

	printk(KERN_INFO "pmcsoftdacm: Found PCI device %p\n", pdev);

	n++;
	if (n > 1)
	  {
	    printk(KERN_ERR "pmcsoftdacm: Cannot handle more than one device, please contact author");
	    return -ENOSYS;
	  }

	err = init(pdev);
	if (err)
	  return err;
      }

    if (n == 0)
      {
	printk(KERN_ERR "pmcsoftdacm: No PMC-SOFTDAC-M devices found.\n");
	return -ENOSYS;
      }
  }

  return 0;
}

/* end file */
