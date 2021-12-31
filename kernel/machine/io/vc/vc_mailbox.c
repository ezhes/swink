#include "vc_mailbox.h"
#include "machine/io/gpio.h"
#include "machine/pmap/pmap.h"
#include "lib/stdio.h"

#define MAILBOX_MESSAGE_MASK 0xFLLU /* 0b1111 */

bool vc_mailbox_call_sync(uint8_t channel, struct vc_mailbox_message *message) {
    uint32_t tx_message = 0;

    /* Wait for the mailbox to be free */
    while (*VC_MAILBOX_STATUS & VC_MAILBOX_FULL);

    /* Must be 16 byte aligned */
    ASSERT((((vm_addr_t)message) & ~MAILBOX_MESSAGE_MASK) == ((vm_addr_t)message));

    tx_message = (pmap_kva_to_pa((vm_addr_t)message) & ~MAILBOX_MESSAGE_MASK) 
                    | (channel & MAILBOX_MESSAGE_MASK);

    *VC_MAILBOX_WRITE = tx_message;

    while (1) {
        /* wait for a response */
        while (*VC_MAILBOX_STATUS & VC_MAILBOX_EMPTY);

        /* reply to our message is up! */
        if (tx_message == *VC_MAILBOX_READ) {
            return message->type == VC_MAILBOX_RESPONSE;
        }
    }
}
