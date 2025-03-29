#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <libwebsockets.h>
#include <string.h>
#include <time.h>

#define MAX_MSG_LEN 512   // tamaño mensaje
#define MAX_CLIENTS 100

struct lws *web_socket = NULL;
const char *username = "usuario1";
int running = 1;

void message_send(const char *type, const char *target, const char *content) {
    if (!web_socket) {
        printf("\n[ERROR] No hay conexión WebSocket activa\n");
        return;
    }

    char message[MAX_MSG_LEN];
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    if (strcmp(type, "register") == 0) {
        snprintf(message, sizeof(message), 
                "{\"type\":\"register\",\"sender\":\"%s\",\"content\":\"%s\"}",
                username, time_str);
    }
    else if (strcmp(type, "broadcast") == 0) {
        snprintf(message, sizeof(message), 
                "{\"type\":\"broadcast\",\"sender\":\"%s\",\"content\":\"%s\",\"timestamp\":\"%s\"}",
                username, content, time_str);
        printf("\n[TÚ] %s [%s]\n", content, time_str);
    }
    else if (strcmp(type, "private") == 0) {
        snprintf(message, sizeof(message), 
                "{\"type\":\"private\",\"sender\":\"%s\",\"target\":\"%s\",\"content\":\"%s\",\"timestamp\":\"%s\"}",
                username, target, content, time_str);
        printf("\n[PRIVADO A %s] %s [%s]\n", target, content, time_str);
    }
    else if (strcmp(type, "list_users") == 0) {
        snprintf(message, sizeof(message), 
                "{\"type\":\"list_users\",\"sender\":\"%s\",\"timestamp\":\"%s\"}",
                username, time_str);
        printf("\n[SOLICITANDO lista de usuarios...]\n");
    }
    else if (strcmp(type, "user_info") == 0) {
        snprintf(message, sizeof(message), 
                "{\"type\":\"user_info\",\"sender\":\"%s\",\"target\":\"%s\",\"timestamp\":\"%s\"}",
                username, target, time_str);
        printf("\n[SOLICITANDO info de %s...]\n", target);
    }
    else if (strcmp(type, "change_status") == 0) {
        if (strcmp(content, "ACTIVO") == 0 || strcmp(content, "OCUPADO") == 0 || 
            strcmp(content, "INACTIVO") == 0) {
            snprintf(message, sizeof(message), 
                    "{\"type\":\"change_status\",\"sender\":\"%s\",\"content\":\"%s\",\"timestamp\":\"%s\"}",
                    username, content, time_str);
            printf("\n[SOLICITANDO cambio de estado a %s...]\n", content);
        } else {
            printf("\n[ERROR] Estado debe ser: ACTIVO, OCUPADO o INACTIVO\n");
            return;
        }
    }
    else if (strcmp(type, "disconnect") == 0) {
        snprintf(message, sizeof(message), 
                "{\"type\":\"disconnect\",\"sender\":\"%s\",\"content\":\"%s\",\"timestamp\":\"%s\"}",
                username, content ? content : "Saliendo", time_str);
    }

    unsigned char buf[LWS_PRE + MAX_MSG_LEN] = {0};
    memcpy(buf + LWS_PRE, message, strlen(message));
    lws_write(web_socket, buf + LWS_PRE, strlen(message), LWS_WRITE_TEXT);
}

// Callback de WebSocket
static int chat_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len) {
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            printf("\n[CONEXIÓN ESTABLECIDA - Registro automático como %s]\n", username);
            web_socket = wsi;
            message_send("register", NULL, "ACTIVO");
            lws_callback_on_writable(wsi);
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE: {
            char buffer[MAX_MSG_LEN + 1];
            strncpy(buffer, (char *)in, len);
            buffer[len] = '\0';

            if (len == 0 || buffer[0] != '{') {
                break;
            }

            printf("%s", buffer);

            if (strstr(buffer, "\"type\": \"register_success\"") != NULL) {
                char sender[100] = {0};
                char content[200] = {0};
                char timestamp[20] = {0};
                
                // Trim any leading/trailing whitespace (like newline characters)
                char *end = buffer + strlen(buffer) - 1;
                while (end > buffer && (*end == ' ' || *end == '\n')) {
                    *end = '\0';
                    --end;
                }

                // Use sscanf to parse the JSON string, with allowance for optional spaces
                int result = sscanf(buffer, 
                    "{\"type\": \"register_success\", \"sender\": \"%[^\"]\", \"content\": \"%[^\"]\", \"timestamp\": \"%[^\"]\"}",
                    sender, content, timestamp);

                if (result == 3) {
                    printf("\n[REGISTRO AUTOMÁTICO EXITOSO] %s [%s]\n", content, timestamp);
                } else {
                    printf("Error parsing JSON: %d values read\n", result);
                }

            }

            else if (strstr(buffer, "\"list_users_response\"") != NULL) {
                printf("Respuesta de lista recibida: %s\n", buffer);
    
                // Buscar el contenido entre "content":[ y ]
                char *content_start = strstr(buffer, "\"content\":[");
                if (content_start) {
                    content_start += 10; // Saltar "\"content\":["
                    char *content_end = strstr(content_start, "]");
                    if (content_end) {
                        int content_len = content_end - content_start;
                        char content[500] = {0};
                        strncpy(content, content_start, content_len);
                        
                        printf("\n═════ USUARIOS CONECTADOS ═════\n");
                        
                        // Procesar la lista de usuarios
                        char *user = strtok(content, ",");
                        while (user != NULL) {
                            // Eliminar comillas y espacios
                            char *ptr = user;
                            while (*ptr == ' ' || *ptr == '"') ptr++;
                            char *end = ptr + strlen(ptr) - 1;
                            while (end > ptr && (*end == ' ' || *end == '"')) end--;
                            *(end + 1) = '\0';
                            
                            if (*ptr) printf("• %s\n", ptr);
                            user = strtok(NULL, ",");
                        }
                        printf("═══════════════════════════════\n");
                    }
                }
            }
            else if (strstr(buffer, "\"type\": \"user_info_response\"") != NULL) {
                // Respuesta con información de un usuario

                printf("%s", buffer);

                char target[100] = {0};
                char ip[100] = "No disponible";
                char status[20] = "No disponible";
                 char timestamp[20] = "No disponible";
                
                sscanf(buffer, "{\"type\":\"user_info_response\"%*[^,],\"target\":\"%[^\"]\",", target);
                sscanf(buffer, "%*[^,]\"content\":{\"ip\":\"%[^\"]\",\"status\":\"%[^\"]\"}", ip, status);
                sscanf(buffer, "%*[^,]\"timestamp\":\"%[^\"]\"", timestamp);

                printf("\n════ INFORMACIÓN DE USUARIO ════\n");
                printf(" Nombre: %s\n", target[0] ? target : "No disponible");
                printf(" Estado: %s\n", status);
                printf(" IP: %s\n", ip);
                printf(" Actualizado: %s\n", timestamp);
                printf("══════════════════════════════\n");
            }
            else if (strstr(buffer, "\"type\": \"broadcast\"") != NULL) {
                // Mensaje broadcast recibido
                char sender[100] = {0};
                char content[200] = {0};
                char timestamp[20] = {0};
                
                if (sscanf(buffer, "{\"type\":\"broadcast\",\"sender\":\"%[^\"]\",\"content\":\"%[^\"]\",\"timestamp\":\"%[^\"]\"}",
                          sender, content, timestamp) == 3) {
                    if (strcmp(sender, username) != 0) {
                        printf("\n[%s] %s [%s]\n", sender, content, timestamp);
                    }
                }
            }
            else if (strstr(buffer, "\"type\": \"private\"") != NULL) {
                // Mensaje privado recibido
                char sender[100] = {0};
                char target[100] = {0};
                char content[200] = {0};
                char timestamp[20] = {0};
                
                if (sscanf(buffer, "{\"type\":\"private\",\"sender\":\"%[^\"]\",\"target\":\"%[^\"]\",\"content\":\"%[^\"]\",\"timestamp\":\"%[^\"]\"}",
                          sender, target, content, timestamp) == 4) {
                    printf("\n[PRIVADO de %s] %s [%s]\n", sender, content, timestamp);
                }
            }
            else if (strstr(buffer, "\"type\": \"status_update\"") != NULL) {
                // Actualización de estado de un usuario
                char user[100] = {0};
                char new_status[20] = {0};
                char timestamp[20] = {0};
                
                if (sscanf(buffer, "{\"type\":\"status_update\",\"sender\":\"%*[^\"]\","
                           "\"content\":{\"user\":\"%[^\"]\",\"status\":\"%[^\"]\"},\"timestamp\":\"%[^\"]\"}",
                          user, new_status, timestamp) == 3) {
                    printf("\n[ESTADO ACTUALIZADO] %s ahora está %s [%s]\n", 
                          user, new_status, timestamp);
                }
            }
            else if (strstr(buffer, "\"type\": \"user_disconnected\"") != NULL) {
                char sender[100] = {0};
                char content[200] = {0};
                char timestamp[20] = {0};
                
                if (sscanf(buffer, "{\"type\":\"user_disconnected\",\"sender\":\"%[^\"]\",\"content\":\"%[^\"]\",\"timestamp\":\"%[^\"]\"}",
                          sender, content, timestamp) == 3) {
                    printf("\n[DESCONEXIÓN] %s [%s]\n", content, timestamp);
                }
            }
            else if (strstr(buffer, "\"type\": \"error\"") != NULL) {
                char sender[100] = {0};
                char content[200] = {0};
                char timestamp[20] = {0};
                
                if (sscanf(buffer, "{\"type\":\"error\",\"sender\":\"%[^\"]\",\"content\":\"%[^\"]\",\"timestamp\":\"%[^\"]\"}",
                          sender, content, timestamp) == 3) {
                    printf("\n[ERROR] %s [%s]\n", content, timestamp);
                }
            }
            break;
        }

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            break;

        case LWS_CALLBACK_CLOSED:
            printf("\n[CONEXIÓN CERRADA]\n");
            running = 0;
            break;

        default:
            break;
    }
    return 0;
}

void show_help() {
    printf("\n═════ COMANDOS DISPONIBLES ═════\n");
    printf("/help               Muestra esta ayuda\n");
    printf("/list               Lista usuarios conectados\n");
    printf("/info <usuario>     Muestra información de un usuario\n");
    printf("/status <ESTADO>    Cambia tu estado (ACTIVO/OCUPADO/INACTIVO)\n");
    printf("/msg <usuario> <msg> Envía mensaje privado\n");
    printf("/exit               Sale del programa\n");
    printf("════════════════════════════════\n");
}

void *receive_messages(void *context) {
    while (running) {
        lws_service((struct lws_context *)context, 50);
    }
    return NULL;
}

static struct lws_protocols protocols[] = {
    {"chat-protocol", chat_callback, 0, MAX_MSG_LEN},
    {NULL, NULL, 0, 0}
};

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <nombredeusuario> <IPdelservidor> <puertodelservidor>\n", argv[0]);
        return 1;
    }
    username = argv[1];
    const char *server_ip = argv[2];
    int server_port = atoi(argv[3]);

    struct lws_context_creation_info info;
    struct lws_client_connect_info ccinfo;
    struct lws_context *context;
    pthread_t thread;

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Error al crear el contexto WebSocket\n");
        return 1;
    }

    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = context;
    ccinfo.address = server_ip;
    ccinfo.port = server_port;
    ccinfo.path = "/";
    ccinfo.host = lws_canonical_hostname(context);
    ccinfo.origin = "origin";
    ccinfo.protocol = protocols[0].name;
    ccinfo.ssl_connection = 0;

    printf("Conectando al servidor %s:%d como %s...\n", server_ip, server_port, username);
    if (!lws_client_connect_via_info(&ccinfo)) {
        fprintf(stderr, "Error al conectar con el servidor\n");
        lws_context_destroy(context);
        return 1;
    }

    // Pequeña pausa para estabilizar la conexión
    usleep(100000);

    pthread_create(&thread, NULL, receive_messages, context);

    show_help();

    while (running) {
        char command[MAX_MSG_LEN];
        printf("\n[%s] > ", username);
        fgets(command, MAX_MSG_LEN, stdin);
        command[strcspn(command, "\n")] = '\0';

        if (command[0] == '/') {
            if (strncmp(command, "/help", 5) == 0) {
                show_help();
            }
            else if (strncmp(command, "/list", 5) == 0) {
                message_send("list_users", NULL, NULL);
            }
            else if (strncmp(command, "/info", 5) == 0) {
                char *target = strtok(command + 6, " ");
                if (target) {
                    message_send("user_info", target, NULL);
                } else {
                    printf("\n[ERROR] Debes especificar un usuario\n");
                }
            }
            else if (strncmp(command, "/status", 7) == 0) {
                char *new_status = strtok(command + 8, " ");
                if (new_status && (strcmp(new_status, "ACTIVO") == 0 || 
                    strcmp(new_status, "OCUPADO") == 0 || strcmp(new_status, "INACTIVO") == 0)) {
                    message_send("change_status", NULL, new_status);
                } else {
                    printf("\n[ERROR] Estado no válido. Usa: ACTIVO, OCUPADO o INACTIVO\n");
                }
            }
            else if (strncmp(command, "/msg", 4) == 0) {
                char *target = strtok(command + 5, " ");
                char *message = strtok(NULL, "");
                if (target && message) {
                    message_send("private", target, message);
                } else {
                    printf("\n[ERROR] Formato: /msg <usuario> <mensaje>\n");
                }
            }
            else if (strncmp(command, "/exit", 5) == 0) {
                message_send("disconnect", NULL, "Saliendo");
                running = 0;
            }
            else {
                printf("\n[ERROR] Comando no reconocido. Escribe /help para ayuda.\n");
            }
        } else if (strlen(command) > 0) {
            message_send("broadcast", NULL, command);
        }
    }

    pthread_join(thread, NULL);
    lws_context_destroy(context);
    printf("\nCliente terminado.\n");
    return 0;
}