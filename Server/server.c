#include <libwebsockets.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                              void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_ESTABLISHED:
            printf("Client connected\n");
            break;

        case LWS_CALLBACK_RECEIVE: {
            printf("Received message: %.*s\n", (int)len, (char *)in);

            // Prepare message with LWS_PRE padding
            unsigned char buf[LWS_PRE + len];
            memcpy(&buf[LWS_PRE], in, len);

            // Send back the message
            if (lws_write(wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT) < 0) {
                printf("Error sending message back to client\n");
            }
            break;
        }

        case LWS_CALLBACK_CLOSED:
            printf("Client disconnected\n");
            break;

        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    {
        "chat-protocol",  // Protocol name (must match client)
        callback_websocket,
        0,  // Per-session data size (not used)
        4096 // Max message size
    },
    { NULL, NULL, 0, 0 } // End of protocols
};

int main() {
    struct lws_context_creation_info info;
    struct lws_context *context;

    memset(&info, 0, sizeof(info));
    info.port = 9000; // Server listens on port 9000
    info.protocols = protocols;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Failed to create WebSocket context\n");
        return -1;
    }

    printf("WebSocket server started on port %d\n", info.port);

    // Event loop to process WebSocket events
    while (1) {
        lws_service(context, 100); // Run event loop
    }

    lws_context_destroy(context);
    return 0;
}
