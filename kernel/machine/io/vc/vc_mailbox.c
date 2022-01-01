#include "vc_mailbox.h"
#include "machine/io/gpio.h"
#include "machine/pmap/pmap.h"
#include "lib/stdio.h"

#define MAILBOX_MESSAGE_MASK 0xFLLU /* 0b1111 */

#define VIDEOCORE_TRACE      (0)

#if VIDEOCORE_TRACE
static inline void trace_message(struct vc_mailbox_message *message, bool tx) {
    printf(
        "[videocore] %s message %p (type = %08x, length = %08x):\n",
        tx ? "tx" : "rx",
        message, message->type, message->length
    );
    for (int i = 0; i < VC_MAILBOX_BODY_COUNT; i++) {
        printf("%08x\n", message->body[i]);
    }
}
#endif

bool vc_mailbox_call_sync(uint8_t channel, struct vc_mailbox_message *message) {
    uint32_t tx_message = 0;

    /* Wait for the mailbox to be free */
    while (*VC_MAILBOX_STATUS & VC_MAILBOX_FULL);

    /* Must be 16 byte aligned */
    ASSERT((((vm_addr_t)message) & ~MAILBOX_MESSAGE_MASK) == ((vm_addr_t)message));

#if VIDEOCORE_TRACE
    trace_message(message, true /* TX */);
#endif


    tx_message = (pmap_kva_to_pa((vm_addr_t)message) & ~MAILBOX_MESSAGE_MASK) 
                    | (channel & MAILBOX_MESSAGE_MASK);

    *VC_MAILBOX_WRITE = tx_message;

    while (1) {
        /* wait for a response */
        while (*VC_MAILBOX_STATUS & VC_MAILBOX_EMPTY);

        /* reply to our message is up! */
        if (tx_message == *VC_MAILBOX_READ) {
#if VIDEOCORE_TRACE
            trace_message(message, false /* TX */);
#endif
            return message->type == VC_MAILBOX_RESPONSE;
        }
    }
}
