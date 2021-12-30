#ifndef PMC_H
#define PMC_H

/** Instruct the Power Management Controller to reboot the device */
void pmc_reset(void);

/** Instruct the Power Management Controller to shutdown the device */
void pmc_shutdown(void);

#endif /* PMC_H */