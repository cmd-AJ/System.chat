#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <string.h>
#include <ncurses.h>

#define MAX_MSG_LEN 256   // tamaño mensaje
#define LWS_PRE_PADDING  LWS_PRE

//ventana ?
WINDOW *win_chat, *win_user, *win_input;
struct lws *web_socket = NULL;

void init_ui_chat(){
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    int height, width;
    getmaxyx(stdscr, height, width);
    //tamaño pantallas
    win_chat = newwin(height-5, width*0.75, 0 ,0);
    win_user = newwin(height-5, width*0.25, 0, width*0.75);
    win_input = newwin(5, width, height-5, 0);

    scrollok(win_chat, TRUE);
    box(win_chat, 0,0);
    box(win_user, 0, 0);
    box(win_input, 0,0);

    mvwprintw(win_chat, 0, 2, "-- Chat --");
    mvwprintw(win_user, 0, 2, "-- Usuarios --");
    mvwprintw(win_input, 0, 2, "-- Entrada --");
    
    wrefresh(win_chat);
    wrefresh(win_user);
    wrefresh(win_input);
}
//lista "en linea"
void connected_list(const char *users[]){
    werase(win_user);
    box(win_user, 0, 0);
    mvwprintw(win_user, 0, 2, "-- Usuarios --");
    for(int i = 0; users[i] != NULL; i++){
        mvwprintw(win_user, i+1, 1, "%s", users[i]);
    }
    wrefresh(win_user);
}
//mandar mensage
void message_text(const char *message){
    wprintw(win_chat, "%s\n", message);
    wrefresh(win_chat);
}

void show_help(){
    message_text("---COMANDOS---");
    message_text("/help -- Ayuda");
    message_text("/msg [usuario] [mensaje] -- mensaje privado");
    message_text("/exit -- salir de programa");
}

//CONN SERVER
static int chat_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len){
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Conexión establecida con el servidor WebSocket\n");
            lws_callback_on_writable(wsi);  // Request to write immediately
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: //registro del mensaje (servidor)
            printf("Mensaje recibido: %.*s\n", (int)len, (char *)in);
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE: { //mensake enviado
            const char *msg = "{\"type\": \"register\", \"sender\": \"usuario1\"}";
            size_t msg_len = strlen(msg);
            unsigned char buf[LWS_PRE + msg_len];  // Buffer LWS padding
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

void message_send(const char *type, const char *target, const char *content){
    char message[MAX_MSG_LEN];
    if(target){
        snprintf(message, sizeof(message), "{\"type\": \"%s\", \"sender\": \"usuario1\", \"target\": \"%s\", \"content\": \"%s\"}", type, target, content);
    } else{
        snprintf(message, sizeof(message), "{\"type\": \"%s\", \"sender\": \"usuario1\", \"content\": \"%s\"}", type, content);
    }
    unsigned char buf[LWS_PRE_PADDING + MAX_MSG_LEN];
    memset(buf, 0, sizeof(buf));
    memcpy(buf+ LWS_PRE_PADDING, message, strlen(message));

    lws_write(web_socket, buf + LWS_PRE_PADDING, strlen(message), LWS_WRITE_TEXT);
}

static const struct lws_protocols protocols[] = {
    { "chat-protocol", callback_chat, 0, 4096 },
    { NULL, NULL, 0, 0 }
};

int main(void) {
    init_ui_chat();
    show_help();

    struct lws_context_creation_info info={0};
    struct lws_client_connect_info ccinfo={0};
    struct lws_context *context = lws_create_context(&info);
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