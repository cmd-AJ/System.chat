#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libwebsockets.h>
#include "hash.h"


static int callback_chat(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Conexión establecida con el servidor WebSocket 101\n");
            lws_callback_on_writable(wsi);  // Request to write immediately
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:
            printf("Mensaje recibido: %.*s\n", (int)len, (char *)in);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: {
            const char *msg = "{\"type\": \"disconnect\", \"sender\": \"PAPI\", \"content\": \"null\"}";
            size_t msg_len = strlen(msg);
            unsigned char buf[LWS_PRE + msg_len];  // Buffer with LWS padding
            memcpy(&buf[LWS_PRE], msg, msg_len);

            lws_write(wsi, &buf[LWS_PRE], msg_len, LWS_WRITE_TEXT);
            break;
        }

        case LWS_CALLBACK_CLOSED:
            printf("Conexión cerrada\n");
            break;

        default:
            break;
    }
    return 0;
}

static const struct lws_protocols protocols[] = {
    { "chat-protocol", callback_chat, 0, 4096 },
    { NULL, NULL, 0, 0 }
};

int main(void) {
    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    struct lws_context *context;
    struct lws *wsi;

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;  // No listening server
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Error al crear el contexto de libwebsockets\n");
        return -1;
    }

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = "localhost";  // Change to your WebSocket server address
    ccinfo.port = 9000;            // Change to the server port
    ccinfo.path = "/";
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0;

    wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        fprintf(stderr, "Error al conectar con el servidor WebSocket\n");
        lws_context_destroy(context);
        return -1;
    }

    printf("Conectando al servidor...\n");

    while (1) {
        lws_service(context, 100);  // Run event loop
    }

    lws_context_destroy(context);
    return 0;
}