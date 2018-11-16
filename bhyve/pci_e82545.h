#ifndef _PCI_E82545_H_
#define _PCI_E82545_H_

#include <dev/altera/fifo/fifo.h>

void e82545_tx_poll(void);
void e82545_rx_poll(void *arg);

int e82545_setup_fifo(struct altera_fifo_softc *fifo_tx,
    struct altera_fifo_softc *fifo_rx);

#endif	/* !_PCI_E82545_H_ */
