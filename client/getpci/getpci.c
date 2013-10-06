/* encoding: UTF-8 */

#include <pci/pci.h>
#include <stdio.h>

int main(void){
    struct pci_access *pacc;
    struct pci_dev *dev;
    u8 htype, revision, prog_class;
    u16 sv_id, sd_id;

    pacc = pci_alloc();		/* Get the pci_access structure */
    /* Set all options you want -- here we stick with the defaults */
    pci_init(pacc);		/* Initialize the PCI library */
    pci_scan_bus(pacc);		/* We want to get the list of devices */
    for (dev=pacc->devices; dev; dev=dev->next){	/* Iterate over all devices */
        pci_fill_info(dev, PCI_FILL_IDENT | PCI_FILL_CLASS);	/* Fill in header info we need */
        htype = pci_read_byte(dev, PCI_HEADER_TYPE)  & 0x7f;				/* Read config register directly */
        if (htype == PCI_HEADER_TYPE_NORMAL){
            sv_id = pci_read_word(dev, PCI_SUBSYSTEM_VENDOR_ID);
            sd_id = pci_read_word(dev, PCI_SUBSYSTEM_ID);
        }
        else if (htype == PCI_HEADER_TYPE_CARDBUS){
            sv_id = pci_read_word(dev, PCI_CB_SUBSYSTEM_VENDOR_ID);
            sd_id = pci_read_word(dev, PCI_CB_SUBSYSTEM_ID);
        }
        else{
            sv_id = sd_id = 0xffff;
        }
        revision = pci_read_byte(dev, PCI_REVISION_ID);
        prog_class = pci_read_byte(dev, PCI_CLASS_PROG);
        printf("vendor=%04x device=%04x subsys=%04x_%04x revision=%02x class=%04x pr_cl=%02x\n",
        dev->vendor_id, dev->device_id, sv_id, sd_id, revision, dev->device_class, prog_class);
    }
    pci_cleanup(pacc);		/* Close everything */
    return 0;
}
