#include "vc_functions.h"
#include "vc_mailbox.h"
#include "lib/string.h"
#include "lib/stdio.h"

/* https://github.com/raspberrypi/firmware/wiki/Mailbox-property-interface */

/* VideoCore */
#define VC_TAG_GET_FIRMWARE_REVISION    (0x00000001)
#define VC_TAG_GET_BOARD_MODEL          (0x00010001)
#define VC_TAG_GET_BOARD_REVISION       (0x00010002)
#define VC_TAG_GET_BOARD_MAC_ADDRESS    (0x00010003)
#define VC_TAG_GET_BOARD_SERIAL         (0x00010004)
#define VC_TAG_GET_ARM_MEMORY_REGION    (0x00010005)
#define VC_TAG_GET_VC_MEMORY_REGION     (0x00010006)
#define VC_TAG_GET_CLOCKS               (0x00010007)

/* Config */
#define VC_TAG_GET_COMMAND_LINE_STR     (0x00050001)

/* Shared resources */
#define VC_TAG_GET_DMA_CHANNELS         (0x00060001)

/* Power */
#define VC_DEVICE_SD_CARD               (0x00000000)
#define VC_DEVICE_UART0                 (0x00000001)
#define VC_DEVICE_UART1                 (0x00000002)
#define VC_DEVICE_USB_HCD               (0x00000003)
#define VC_DEVICE_I2C0                  (0x00000004)
#define VC_DEVICE_I2C1                  (0x00000005)
#define VC_DEVICE_I2C2                  (0x00000006)
#define VC_DEVICE_SPI                   (0x00000007)
#define VC_DEVICE_CCP2TX                (0x00000008)
#define VC_DEVICE_UNK1                  (0x00000009)
#define VC_DEVICE_UNK2                  (0x0000000a)

#define VC_TAG_GET_POWER_STATE          (0x00020001)
#define VC_TAG_GET_POWER_ON_WAIT_MS     (0x00020002)
#define VC_TAG_SET_POWER_STATE          (0x00028001)

/* Clocks */
#define VC_TAG_GET_CLOCK_STATE              (0x00030001)
#define VC_TAG_SET_CLOCK_STATE              (0x00038001)
#define VC_TAG_GET_CLOCK_RATE               (0x00030002)
#define VC_TAG_GET_MEASURED_CLOCK_RATE      (0x00030047)
#define VC_TAG_SET_CLOCK_RATE               (0x00038002)

/* On-board LED */
#define VC_TAG_GET_LED_STATUS               (0x00030041)
#define VC_TAG_TEST_ONBOARD_LED_STATUS      (0x00034041)
#define VC_TAG_SET_LED_STATUS               (0x00038041)

/* Parse the response length from a VC tag to bytes */
#define VC_TAG_GET_RESPONSE_LENGTH(tag) \
    ((tag->response_buffer_length) & ~(1U<<31))

/** 
 * Represents a tag which is embedded in a message
 * Use `message_add_tag` to insert this
 */ 
struct vc_message_tag {
    uint32_t tag;

    /** The actual allocated buffer length */
    uint32_t request_buffer_length;

    /** 
     * The number of bytes the VC wanted to send (may be more than 
     * request_buffer_length)!
     */
    uint32_t response_buffer_length;

    /** The underlying response buffer storage */
    uint32_t buffer[0];
};

/** Initialize a pre-allocated message */
static inline void
message_init(struct vc_mailbox_message *message) {
    memset(message, 0x00, sizeof(*message));
    message->type = MAILBOX_MESSAGE_TYPE_REQUEST;
    message->length = sizeof(uint32_t) * (2 /* start header */ + 1 /* end tag */);
}


/** 
 * Add a tag to a message and return a pointer to the new tag. NULL is returned 
 * if the message buffer ran out of space.
 */
static struct vc_message_tag *
message_add_tag(struct vc_mailbox_message *message, 
                uint32_t tag_type, 
                uint32_t slots) {
    struct vc_message_tag *tag = NULL;
    uint32_t used_slots = (message->length / sizeof(uint32_t)) - 3;

    if (used_slots + 3 /* tag headers */ + slots >= VC_MAILBOX_BODY_COUNT) {
        /* not enough space to add the tag and terminator */
        return NULL;
    }

    /* allocate space for tag */
    message->length += sizeof(uint32_t) * (3 /* tag headers */ + slots);
    /* configure tag */
    tag = (struct vc_message_tag *)(message->body + used_slots);
    tag->tag = tag_type;
    tag->request_buffer_length = sizeof(uint32_t) * slots;
    /* do not need to init other fields as entire structure is zeroed */
    return tag;
}

bool vc_functions_get_arm_memory_region(phys_addr_t *addr, uint32_t *size) {
    struct vc_mailbox_message message;
    struct vc_message_tag *memory_region_tag = NULL;

    message_init(&message);

    if (!(memory_region_tag = message_add_tag(
                                            &message, 
                                            VC_TAG_GET_ARM_MEMORY_REGION, 
                                            /* slots */ 2))) {
        return false;
    }

    if (!vc_mailbox_call_sync(MAILBOX_CHANNEL_AP_TO_VC, &message)) {
        return false;
    }

    if (VC_TAG_GET_RESPONSE_LENGTH(memory_region_tag) < sizeof(uint32_t) * 2) {
        /* response too short? */
        return false;
    }

    *addr = memory_region_tag->buffer[0];
    *size = memory_region_tag->buffer[1];

    return true;
}

static bool get_tag_32(uint32_t tag_type, uint32_t *out) {
    struct vc_mailbox_message message;
    struct vc_message_tag *tag = NULL;

    message_init(&message);

    if (!(tag = message_add_tag(&message, tag_type, /* slots */ 1))) {
        return false;
    }

    if (!vc_mailbox_call_sync(MAILBOX_CHANNEL_AP_TO_VC, &message)) {
        return false;
    }

    if (VC_TAG_GET_RESPONSE_LENGTH(tag) < sizeof(uint32_t) * 1) {
        /* response too short? */
        return false;
    }

    *out = tag->buffer[0];

    return true;
}

bool vc_functions_get_board_model(uint32_t *model) {
    return get_tag_32(VC_TAG_GET_BOARD_MODEL, model);
}

bool vc_functions_get_board_revision(uint32_t *revision) {
    return get_tag_32(VC_TAG_GET_BOARD_REVISION, revision);
}

bool vc_functions_get_vc_revision(uint32_t *revision) {
    return get_tag_32(VC_TAG_GET_FIRMWARE_REVISION, revision);
}

bool vc_functions_set_activity_led(bool enabled) {
    struct vc_mailbox_message message;
    struct vc_message_tag *tag = NULL;

    message_init(&message);

    if (!(tag = message_add_tag(&message, 
                                VC_TAG_SET_LED_STATUS, 
                                /* slots */ 2))) {
        return false;
    }
    tag->buffer[0] = 130 /* status LED */;
    tag->buffer[1] = enabled;

    if (!vc_mailbox_call_sync(MAILBOX_CHANNEL_AP_TO_VC, &message)) {
        return false;
    }

    return true;
}
