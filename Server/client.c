#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libwebsockets.h>
#include <string.h>

#define MAX_MSG_LEN 512   // tamaño mensaje
struct lws *web_socket = NULL;
const char *username = "usuario1";
int running = 1; //cierre programa

//envio de mensajes al serv
void message_send(const char *type, const char *target, const char *content){
    char message[MAX_MSG_LEN];
    //hora actual
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info); //formato tiempo 
    //publicos
    if (strcmp(type, "public") == 0) {
        snprintf(message, sizeof(message), "%s", content);
        //Mostrar mensaje con nom de ususario y tiempo
        printf("%s: %s [%s]\n", username, content, time_str);
    } else {
        //privados y otros
        snprintf(message, sizeof(message), "{\"type\": \"%s\", \"sender\": \"%s\", \"target\": \"%s\", \"content\": \"%s\", \"timestamp\": \"%s\"}", 
                 type, username, target ? target : "", content ? content : "", time_str);
    }

    //enviar menmsaje
    unsigned char buf[LWS_PRE + MAX_MSG_LEN];
    memset(buf, 0, sizeof(buf));
    memcpy(buf+ LWS_PRE, message, strlen(message));

    lws_write(web_socket, buf + LWS_PRE, strlen(message), LWS_WRITE_TEXT); //al server
}
//estados
void status(const char *new_status){
    message_send("status", NULL, new_status);
}
//conectados
void list_users() {
    message_send("list", NULL, NULL);
}
//info ususario
void user_info(const char *target) {
    message_send("info", target, NULL);
}
void message_text(const char *message) {
    printf("%s\n", message);
}

//ayuda
void show_help(){
    message_text("-- Comandos --");
    message_text("    /help -- Ayuda    ");
    message_text("    /msg [usuario] [mensaje] -- mensaje privado   ");
    message_text("    /exit -- salir de programa    ");
    message_text("    /status [estado] -- estado del usuario y cambio de estado    ");
    message_text("    /list -- lista usuarios   ");
    message_text("    /info [usuario] -- informacion usuarios    ");
}
//recibir mensajes
void *receive_messages(void *context) {
    while (running) {
        lws_service((struct lws_context *)context, 0); //escucha del server
    }
    return NULL;
}


//SERVER (maneja conexion, recibir mensajes, cierre de sesion, etc)
static int chat_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len){
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("Conexión establecida con el servidor WebSocket\n");
            web_socket = wsi;
            
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:{ //registro del mensaje (servidor)
            char buffer[MAX_MSG_LEN];
            strncpy(buffer, (char *)in, len);
            buffer[len] = '\0';

            char type[MAX_MSG_LEN], sender[MAX_MSG_LEN], content[MAX_MSG_LEN], timestamp[20];
            
            //parsear JSON
            int parsed = sscanf(buffer, "{\"type\": \"%[^\"]\", \"sender\": \"%[^\"]\", \"content\": \"%[^\"]\", \"timestamp\": \"%[^\"]\"}",
                type, sender, content, timestamp);
            if (parsed == 4) {
                if (strcmp(type, "status") == 0) {
                    printf("Estado de %s: %s\n", sender, content);
                } else {
                    //mostrar mensaje con  nom de usuario y hora en terminal
                    printf("%s: %s [%s]\n", sender, content, timestamp);
                }
            }
            printf("\n");
            }
            break;

        case LWS_CALLBACK_CLOSED:
            printf("Conexión cerrada\n");
            break;

        default:
            break;
    }
    return 0;
}

static const struct lws_protocols protocols[] = {
    { "chat-protocol", chat_callback, 0, 4096 },
    { NULL, NULL, 0, 0 }
};

int main() {
    //iniciar cliente
    struct lws_context_creation_info info={0};
    struct lws_client_connect_info ccinfo={0};
    struct lws_context *context = lws_create_context(&info); //conn
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
    ccinfo.address = "3.147.6.53";  
    ccinfo.port = 9000;           
    ccinfo.path = "/";
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0;

    printf("Conectando al servidor...\n");

    wsi = lws_client_connect_via_info(&ccinfo);
    if (!wsi) {
        fprintf(stderr, "Error al conectar con el servidor WebSocket\n");
        lws_context_destroy(context);
        return -1;
    } else{
        printf("Conexion exitosa con el servidor WebSocket\n");
    }

    pthread_t thread;
    pthread_create(&thread, NULL, receive_messages, (void *)context); //hilo para recibir (permite que el chat se ejecute miesntras esta viendo si recibe mensajes)

    while (1) {
        char command[MAX_MSG_LEN];
        printf(">>> ");
        fgets(command, MAX_MSG_LEN, stdin);
        command[strcspn(command, "\n")] =0;

         if (command[0] != '/'){
            //Publico
            message_send("public", NULL, command);
        }else if (strncmp(command, "/list", 5) == 0) {
            list_users();
        } else if (strncmp(command, "/status", 7) == 0) {
            status(command + 8); //estado nuevo
        }else if (strncmp(command, "/msg", 4) == 0) {
            char *target = strtok(command + 5, " "); //nom uusario destinatario
            char *content = strtok(NULL, ""); //mensaje
            if (target && content) {
                message_send("private", target, content);
            }else {
                printf("Formato Mensaje Privado: /msg <usuario> <mensaje>\n");
            }
        }else if (strncmp(command, "/help", 5) == 0){
            show_help();
        }else if(strncmp(command, "/exit", 5) == 0){
            printf("CERRANDO SESION...\n");
            message_send("disconnect", NULL, "Cierre de sesion");
            running = 0;
            break;
        } else{
            printf("Comando no reconocido. Usa /help para ver los comandos :) \n");
        }
    }
    pthread_cancel(thread);
    pthread_join(thread, NULL);
    lws_context_destroy(context);
    return 0;
}