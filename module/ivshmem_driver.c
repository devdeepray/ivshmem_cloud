/*
 *	Author: Chinmay Satish Kulkarni
 *	Date:	12th September 2015
 *	Place:	I.I.T. Bombay
 *
 *	A Driver for ivshmem (lwn.net/Articles/380869 - Accessed on 12/09/2014)
 *	
 *	Based on the driver presented at:
 *	- nairobi-embedded.org/linux_pci_device_driver.html (12/09/2015)
 */

#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/ratelimit.h>

#define IVSHMEM_MINOR		0
#define IVSHMEM_NUM_DEVS	1
#define IVSHMEM_PCI_BAR1	1
#define IVSHMEM_PCI_BAR2	2

#define IVSHMEM_DEV_IRQ		"ivshmem_irq"
#define IVSHMEM_DEV_NAME 	"ivshmem"

#define INTERRUPT_MASK		0x00
#define INTERRUPT_STATUS	0x04

/*
 * 	ivshmem_dev_t:
 * 	Contains all important information about the ivshmem device
 */
static struct ivshmem_dev_t {
	/* Members related to the ivshmem /dev node */
	dev_t		ivshmem_dev;
	struct cdev 	ivshmem_cdev;
	struct class 	*ivshmem_sysfs_class;
	struct device 	*ivshmem_device;

	/* Members related to the data mmio region */
	resource_size_t	data_mmio_start;
	resource_size_t data_mmio_len;

	/* Members related to the control registers */
	void* __iomem	regs_base_addr;
} ivshmem_pci;

/***************************************************************** PCI DEVICE */

/*
 * 	ivshmem_pci_interrupt(int, void*):
 * 	Dummy interrupt handler for ivshmem
 */
static irqreturn_t ivshmem_pci_interrupt(int irq, void *device_id)
{
	u32 status;

	status = readl(ivshmem_pci.regs_base_addr + INTERRUPT_STATUS);
	if ((status == 0) || (status == 0xffffffff))
		return (IRQ_NONE);

	pr_info_ratelimited("Interrupt (status = 0x%04x)\n", status);
	return (IRQ_HANDLED);
}

/*
 * 	ivshmem_pci_probe(struct pci_dev, const struct pci_device_id):
 * 	Setup the pci device
 * 	- Enable the pci device
 * 	- Mark ownership over BAR 2
 * 	- Get the ivshmem start address and length
 * 	- Register an interrupt handler
 */
static int
ivshmem_pci_probe(struct pci_dev *pcidev, const struct pci_device_id *pciid)
{
	int result = 0;
	struct device* dev = NULL;
	const char *device_name = NULL;

	dev = &(pcidev->dev);
	device_name = pci_name(pcidev);

	result = pci_enable_device(pcidev);
	if (result < 0) {
		dev_err(dev, "Enable error %d for pci device\n", result);
		goto exit;
	}

	result = pci_request_region(pcidev, IVSHMEM_PCI_BAR1, IVSHMEM_DEV_NAME);
	if (result == -EBUSY) {
		dev_err(dev, "Request error %d on bar %d\n" ,result,
			IVSHMEM_PCI_BAR1);
		goto pci_disable;
	}

	result = pci_request_region(pcidev, IVSHMEM_PCI_BAR2, IVSHMEM_DEV_NAME);
	if (result == -EBUSY) {
		dev_err(dev, "Request error %d on bar %d\n" ,result,
			IVSHMEM_PCI_BAR2);
		goto pci_release;
	}

	ivshmem_pci.data_mmio_start = pci_resource_start(pcidev, 2);
	ivshmem_pci.data_mmio_len = pci_resource_len(pcidev, 2);

	ivshmem_pci.regs_base_addr = pci_iomap(pcidev, 0, 0x100);
	if (ivshmem_pci.regs_base_addr == NULL) {
		dev_err(dev, "pci_iomap error %d for pci device\n", result);
		goto pci_release;
	}

	writel(0xffffffff, ivshmem_pci.regs_base_addr + INTERRUPT_MASK);
	result = request_irq(pcidev->irq, ivshmem_pci_interrupt, IRQF_SHARED,
			IVSHMEM_DEV_NAME, IVSHMEM_DEV_IRQ);
	if (result) {
		dev_err(dev, "Failed to register irq handler");
		goto pci_iounmap;
	}

	dev_info(dev, "Setup device\n");

	return (0);

pci_iounmap:
	pci_iounmap(pcidev, ivshmem_pci.regs_base_addr);

pci_release:
	pci_release_regions(pcidev);

pci_disable:
	pci_disable_device(pcidev);

exit:
	return (result);
}

/*
 * 	ivshmem_pci_remove(struct pci_dev*):
 * 	Disable the pci device
 * 	- Unregister the irq handler
 * 	- Release all ownerships
 * 	- Disable the device
 */
static void ivshmem_pci_remove(struct pci_dev* pcidev)
{
	free_irq(pcidev->irq, IVSHMEM_DEV_IRQ);
	pci_release_regions(pcidev);
	pci_disable_device(pcidev);
	dev_info(&(pcidev->dev), "Disabled device\n");
}

DEFINE_PCI_DEVICE_TABLE(ivshmem_pci_table) = { { PCI_DEVICE(0x1af4, 0x1110) },
	{ 0 } };
MODULE_DEVICE_TABLE(pci, ivshmem_pci_table);

static struct pci_driver ivshmem_pci_driver = {
	.name		= IVSHMEM_DEV_NAME,
	.id_table	= ivshmem_pci_table,
	.probe		= ivshmem_pci_probe,
	.remove		= ivshmem_pci_remove,
};

/*********************************************************** CHARACTER DEVICE */

static int ivshmem_cdev_open(struct inode* inode, struct file* filp)
{
	return (0);
}

/*
 * 	ivshmem_cdev_mmap(struct file*, struct vm_area_struct*):
 * 	The ivshmem character device's mmap routine.
 * 	- Check if the shared memory size = the vma's size
 * 	- Disable caching on the vm_area_struct
 * 	- Build page tables mapping the vma to the ivshmem pci device
 */
static int ivshmem_cdev_mmap(struct file* filp, struct vm_area_struct* vma)
{
	int result = 0;
	unsigned long offset = 0;
	unsigned long physical = 0;
	unsigned long phys_pfn = 0;
	unsigned long virt_size = 0;
	unsigned long phys_size = 0;

	offset = vma->vm_pgoff << PAGE_SHIFT;
	physical = (unsigned long)(ivshmem_pci.data_mmio_start) + offset;
	phys_pfn = physical >> PAGE_SHIFT;

	virt_size = vma->vm_end - vma->vm_start;
	phys_size = ivshmem_pci.data_mmio_len - offset;
	if (virt_size != phys_size)
		return (-EINVAL);

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	result = io_remap_pfn_range(vma, vma->vm_start, phys_pfn, virt_size,
			vma->vm_page_prot);
	if (result)
		return (-EINVAL);

	return (0);
}

static int ivshmem_cdev_release(struct inode* inode, struct file* filp)
{
	return (0);
}

static const struct file_operations ivshmem_cdev_fops = {
	.owner 		= THIS_MODULE,
	.open		= ivshmem_cdev_open,
	.mmap		= ivshmem_cdev_mmap,
	.release	= ivshmem_cdev_release,
};

/********************************************************** LKM INIT AND EXIT */

/*
 * 	ivshmem_init(void):
 * 	The module's init routine.
 * 	- Create a character device for ivshmem
 * 	- Create a /dev node for ivshmem
 * 	- Register the pci driver
 */
static int ivshmem_init(void)
{
	int result = 0;

	result = alloc_chrdev_region(&(ivshmem_pci.ivshmem_dev), IVSHMEM_MINOR,
		IVSHMEM_NUM_DEVS, IVSHMEM_DEV_NAME);
	if (result < 0) {
		printk(KERN_ALERT "ivshmem_driver: alloc_chrdev_region"
		" failed\n");
		goto exit;
	}

	cdev_init(&(ivshmem_pci.ivshmem_cdev), &ivshmem_cdev_fops);
	result = cdev_add(&(ivshmem_pci.ivshmem_cdev), ivshmem_pci.ivshmem_dev,
		IVSHMEM_NUM_DEVS);
	if (result < 0) {
		printk(KERN_ALERT "ivshmem_driver: cdev_add failed\n");
		goto unregister_ch;
	}

	ivshmem_pci.ivshmem_sysfs_class = class_create(THIS_MODULE,
		IVSHMEM_DEV_NAME);
	if (ivshmem_pci.ivshmem_sysfs_class == NULL) {
		printk(KERN_ALERT "ivshmem_driver: class_create failed\n");
		goto destroy_chard;
	}

	ivshmem_pci.ivshmem_device = device_create(ivshmem_pci.ivshmem_sysfs_class,
		NULL, ivshmem_pci.ivshmem_dev, NULL, IVSHMEM_DEV_NAME);
	if (ivshmem_pci.ivshmem_device == NULL) {
		printk(KERN_ALERT "ivshmem_driver: device_create failed\n");
		goto destroy_class;
	}

	result = pci_register_driver(&ivshmem_pci_driver);
	if (result < 0) {
		printk(KERN_ALERT "ivshmem_driver: Failed to register the pci"
		" driver\n");
		goto destroy_devic;
	}

	return (0);

destroy_devic:
	device_destroy(ivshmem_pci.ivshmem_sysfs_class,
	ivshmem_pci.ivshmem_dev);

destroy_class:
	class_destroy(ivshmem_pci.ivshmem_sysfs_class);

destroy_chard:
	cdev_del(&(ivshmem_pci.ivshmem_cdev));

unregister_ch:
	unregister_chrdev_region(ivshmem_pci.ivshmem_dev,
	IVSHMEM_NUM_DEVS);

exit:
	return (result);
}

/*
 * 	ivshmem_exit(void):
 * 	The module's exit routine
 * 	- Unregister the pci driver
 * 	- Delete the /dev node
 * 	- Delete the character device
 */ 
static void ivshmem_exit(void)
{
	pci_unregister_driver(&ivshmem_pci_driver);
	device_destroy(ivshmem_pci.ivshmem_sysfs_class, ivshmem_pci.ivshmem_dev);
	class_destroy(ivshmem_pci.ivshmem_sysfs_class);
	cdev_del(&(ivshmem_pci.ivshmem_cdev));
	unregister_chrdev_region(ivshmem_pci.ivshmem_dev, IVSHMEM_NUM_DEVS);
}

module_init(ivshmem_init);
module_exit(ivshmem_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chinmay S. Kulkarni");
MODULE_DESCRIPTION("A driver for ivshmem (Inter VM Shared Memory)");
