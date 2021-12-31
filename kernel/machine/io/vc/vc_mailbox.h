#ifndef VC_MAILBOX_H
#define VC_MAILBOX_H
#include "lib/types.h"

/** Request information from the VC */
#define MAILBOX_CHANNEL_AP_TO_VC (8)
/** VC requesting information from us (currently not implemented in VC */
#define MAILBOX_CHANNEL_VC_TO_AP (9)

/** A message type indicating we are requesting data */
#define MAILBOX_MESSAGE_TYPE_REQUEST (0)

/** Number of body elements in a message */
#define VC_MAILBOX_BODY_COUNT (32)
/** Messages are used for communicating with the videocore */
struct vc_mailbox_message {
    volatile uint32_t length;
    volatile uint32_t type;
    volatile uint32_t body[VC_MAILBOX_BODY_COUNT];
} __attribute__((aligned(16)));

/**
 * @brief Sends a `message` to the VideoCore on `channel`.
 * If successful, returns TRUE. Otherwise, returns FALSE.
 * The response (on both success and error) is written back to `message`
 */
bool vc_mailbox_call_sync(uint8_t channel, struct vc_mailbox_message *message);

#endif /* VC_MAILBOX_H */