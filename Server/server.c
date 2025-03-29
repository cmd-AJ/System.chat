    #include <libwebsockets.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include "hash.h"
    #include <pthread.h>
    #include <time.h>     

    #define MAX_CLIENTS 100
    #define TABLE_SIZE 10
    #define BUFFER_SIZE 1024


    struct session_info {
        struct lws *wsi;           // Store WebSocket instance to identify the client
        char user_id[100];         // Store the user ID
        char ip_address[100];      // Store the client's IP address
        char status[20];
    };

    pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER; 
    // Global array of session info
    static struct session_info *session_table[MAX_CLIENTS] = {0};


    void check_all_users() {
        printf("Performing periodic user check...\n");
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (session_table[i] != NULL) {
                printf("User: %s (IP: %s), STATUS: %s  is still connected\n", 
                       session_table[i]->user_id, session_table[i]->ip_address, session_table[i]->status);
                       strcpy(session_table[i]->status, "INACTIVE");
            }
        }
    }
    

    static int callback_websocket(struct lws *wsi, enum lws_callback_reasons reason,
                                void *user, void *in, size_t len) {
        struct session_info *session;
        int client_index = -1;

        switch (reason) {
            case LWS_CALLBACK_ESTABLISHED:
                printf("Client connected\n");

                // Allocate memory for the session
                session = (struct session_info *)malloc(sizeof(struct session_info));
                if (session == NULL) {
                    printf("Error allocating memory for session\n");
                    return -1;
                }

                session->wsi = wsi;  // Store the WebSocket instance

                // Assign a default user ID (e.g., "guest")
                strcpy(session->user_id, "guest");  // Default user ID
                strcpy(session->status, "ACTIVE");  // Default user ID

                // Get the client's IP address using libwebsockets function
                char ip_address[100];  // Buffer to store IP address
                int ip_len = sizeof(ip_address);
                if (lws_get_peer_simple(wsi, ip_address, ip_len) != NULL) {
                    strncpy(session->ip_address, ip_address, sizeof(session->ip_address) - 1);
                    session->ip_address[sizeof(session->ip_address) - 1] = '\0';  // Ensure null termination
                } else {
                    strcpy(session->ip_address, "Unknown IP");  // Fallback if IP address is not found
                }

                // Find an empty slot in the session_table
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (session_table[i] == NULL) {
                        client_index = i;
                        session_table[i] = session;  // Store the session in the table
                        lws_set_wsi_user(wsi, session);
                        break;
                    }
                }

                if (client_index == -1) {
                    printf("Error: No space for new clients\n");
                    free(session);  // Cleanup in case the session couldn't be added
                }

                printf("Default user created with ID: %s and IP address: %s\n", session->user_id, session->ip_address);
                break;
            
                case LWS_CALLBACK_RECEIVE: {

                    struct session_info *session = (struct session_info *)lws_wsi_user(wsi);
                    if (!session) {
                        
                        printf("{ \"type\": \"error\", \"sender\": \"server\",\"Error Getting  SESSION TO HOST \": \"Descripción del error\",\"timestamp\": \"NA\"}");
                        return -1;
                    }

                    
                
                    struct hash_table table = {0};
                    char *original_message = (char *)in;
                    char *cleaned_message =  (char *)in;

                    clean_json_message(cleaned_message);

                    
                    char *jsonitems[TABLE_SIZE];
        
                    char *par = strtok(cleaned_message, ","); 
                    int counter = 0;
                    while (par != NULL)
                    {
                        jsonitems[counter] = par;
                        counter = counter + 1;
                        par = strtok(NULL, ",");
                    }
                    
                    for (int i = 0; i < counter; i++){
        
                        // Tokenize by colon
                        char *key = strtok(jsonitems[i], ":");
                        char *value = strtok(NULL, ":");
        
                        if (key != NULL && value != NULL) {
                            key = trim(key);
                            remove_quotes(key);
                            value = trim(value);
                            remove_quotes(value);
        
                            insert(&table, key, value);
                        }
                    }

                    


                    //I get the type of it

                    char *type = get(&table, "type");


                    char timestamp[30];
                    gettime(timestamp, sizeof(timestamp));
       
                    

                    if (type != NULL) {
                        
                        if (strcmp(type, "register") == 0)
                        {
                            char *register_user = get(&table, "sender");
                            char ip[128];
                            char name[128];
                            lws_get_peer_addresses(lws_get_network_wsi(wsi), lws_get_socket_fd(wsi), name, sizeof(name), ip, sizeof(ip));

                            // Asignar el nombre de usuario
                            strncpy(session->user_id, register_user, sizeof(session->user_id) - 1);
                            session->user_id[sizeof(session->user_id) - 1] = '\0';
                            
                            // Registrar en archivo
                            pthread_mutex_lock(&file_mutex);
                            FILE *file = fopen("registries.txt", "a");
                            if (file == NULL) {
                                sprintf(cleaned_message, "{ \"type\": \"error\", \"sender\": \"server\", \"error\": \"Error opening file\", \"timestamp\": \"%s\"}", timestamp);
                                perror("Error opening file");
                                pthread_mutex_unlock(&file_mutex);  
                                return 1;
                            }
                            
                            //Nuevo registro
                            fprintf(file, "%s,%s\n", register_user, ip);
                            fclose(file);
                            pthread_mutex_unlock(&file_mutex);
                            
                            //Confirmacion
                            char hostname[256];
                            if (gethostname(hostname, sizeof(hostname)) == 0) {
                                sprintf(cleaned_message, "{\"type\": \"register_success\", \"sender\": \"%s\", \"content\": \"Registro Exitoso\", \"timestamp\": \"%s\"}", 
                                    hostname, timestamp);
                            } else {
                                sprintf(cleaned_message, "{\"type\": \"register_success\", \"sender\": \"server\", \"content\": \"Registro Exitoso\", \"timestamp\": \"%s\"}", 
                                    timestamp);
                            }
                            
                            printf("Nuevo usuario registrado: %s desde IP: %s\n", register_user, ip);
                           
                        }
            

                        if ((strcmp(type, "private") == 0))
                        {
                            char *user = get(&table, "target");
                            char *contenido = get(&table, "content");

                            for (int i = 0; i < MAX_CLIENTS; i++) {
                                if (session_table[i] != NULL) {

                        
                                    // If the user ID matches the target user
                                    if (strcmp(session_table[i]->user_id, user) == 0) {
                                        printf("Sending message to %s\n", session_table[i]->user_id);
                                        
                                        // Prepare the message to send to the target client
                                        char msg[1024];
                                        snprintf(msg, sizeof(msg), "{\"type\": \"private\", \"sender\": \"%s\", \"target\": \"%s\",  \"content\": \"%s\",  \"timestamp\": \"%s\"}", session->user_id, session_table[i]->user_id,contenido, timestamp);
                                        
                                        // Ensure the WebSocket session is valid
                                        if (session_table[i]->wsi != NULL) {
                                            unsigned char buf[LWS_PRE + strlen(msg)];
                                            memcpy(&buf[LWS_PRE], msg, strlen(msg));
                                            
                                            // Send the message to the target client via WebSocket
                                            lws_write(session_table[i]->wsi, &buf[LWS_PRE], strlen(msg), LWS_WRITE_TEXT);
                                            printf("Message sent to target: %s\n", msg);
                                        } else {
                                            printf("Invalid WebSocket session for user: %s\n", session_table[i]->user_id);
                                        }
                                    }
                                }
                            }
                                    
                        }



                        if ((strcmp(type, "broadcast") == 0))
                        {
                            char *user = get(&table, "target");
                            char *contenido = get(&table, "content");

                            for (int i = 0; i < MAX_CLIENTS; i++) {
                                if (session_table[i] != NULL) {

                        
                                    // If the user ID matches the target user
                                 
                                    
                                    // Prepare the message to send to the target client
                                    char msg[1024];
                                    snprintf(msg, sizeof(msg), "{\"type\": \"broadcast\", \"sender\": \"%s\", \"content\": \"%s\",  \"timestamp\": \"%s\"}", session->user_id,contenido, timestamp);
                                    
                                    // Ensure the WebSocket session is valid
                                    if (session_table[i]->wsi != NULL) {
                                        unsigned char buf[LWS_PRE + strlen(msg)];
                                        memcpy(&buf[LWS_PRE], msg, strlen(msg));
                                        
                                        // Send the message to the target client via WebSocket
                                        lws_write(session_table[i]->wsi, &buf[LWS_PRE], strlen(msg), LWS_WRITE_TEXT);
                                        printf("Sent Global Message: %s\n", msg);
                                    } 
                                }
                            }
                                    
                        }


                        if ((strcmp(type, "list_users") == 0))
                        {

                            printf("Solicitud de lista de usuarios recibida de %s\n", session->user_id);

                            char arrayusers[1024] = "[";
                            int first = 1;


                            for (int i = 0; i < MAX_CLIENTS; i++) {
                                if (session_table[i] != NULL && strcmp(session_table[i]->user_id, "guest") != 0) {

                                    if (!first) {
                                        strcat(arrayusers, ",");
                                    }
                                    first = 0;
                                    strcat(arrayusers, "\"");
                                    strcat(arrayusers, session_table[i]->user_id);
                                    strcat(arrayusers, "\"");                                   

                                }
                            }
                            strcat(arrayusers, "]"); 
                            printf("Lista de usuarios a enviar: %s\n", arrayusers);
    
                            sprintf(cleaned_message,"{\"type\":\"list_users_response\",\"sender\":\"server\",\"content\":%s,\"timestamp\":\"%s\"}",
                                arrayusers, timestamp);
                           
                        }


                        if ((strcmp(type, "user_info") == 0)) {
                            char *user_target = get(&table, "target");
                        

                            FILE *file = fopen("registries.txt", "r");  // Open file for reading
                            if (file == NULL) {
                                perror("Error opening file");
                                pthread_mutex_unlock(&file_mutex);  
                                return 1;
                            }

                            char status[30]= "NO DISPONIBLE";
                        
                            char buffer[BUFFER_SIZE];  // Buffer to hold each line
                            while (fgets(buffer, sizeof(buffer), file)) {  // Read one line at a time
                                buffer[strcspn(buffer, "\n")] = 0;  // Remove newline if present
                        
                                char temp_buffer[BUFFER_SIZE];  // Copy of buffer to avoid strtok modifying original
                                strcpy(temp_buffer, buffer);
                        
                                char *user = strtok(temp_buffer, ",");
                                char *ip_address = strtok(NULL, ",");
                                

                                
                        
                                if (user != NULL) {  // Ensure valid tokens
                                    if (strcmp(user_target, user) == 0) {
                                        for (int i = 0; i < MAX_CLIENTS; i++) {
                                            if (session_table[i] != NULL) {
            
                                                if (strcmp(session_table[i]->user_id, user)==0){
                                                     strcpy(status, session_table[i]->status); 
                                                }
                                                
            
                                            }
                                        }

                                    }
                                }
                            }
                        
                            fclose(file);  // Close the file after reading


                            char hostname[256];
                            if (gethostname(hostname, sizeof(hostname)) == 0) {
                                sprintf(cleaned_message, "{\"type\": \"user_info_response\", \"sender\": \"%s\", \"target\": \"%s\", \"content\": { \"ip\": \"%s\", \"status\": \"%s\" }, \"timestamp\": \"%s\"}", hostname, session->user_id, ip_address, status, timestamp);
                            } else {
                                sprintf(cleaned_message, "{ \"type\": \"error\", \"sender\": \"server\", \"Error\": \"Cannot Find Hostname\", \"timestamp\": \"%s\"}", timestamp);
                            }



                        }

                        if ((strcmp(type, "change_status") == 0))
                        {
                            char *user = get(&table, "sender");
                            char *content = get(&table, "content");
                            strcpy(session->status, content); 


                            char hostname[256];
                            if (gethostname(hostname, sizeof(hostname)) == 0) {
                                sprintf(cleaned_message, "{\"type\": \"user_info_response\", \"sender\": \"%s\", \"content\": { \"user\": \"%s\", \"status\": \"%s\"  }, \"timestamp\": \"%s\"}", hostname,session->user_id, session->status ,timestamp);
                            } else {
                                sprintf(cleaned_message, "{ \"type\": \"error\", \"sender\": \"server\", \"Error\": \"Cannot Find Hostname\", \"timestamp\": \"%s\"}", timestamp);
                            }
                           
                        }

                        if (strcmp(type, "disconnect") == 0) {
                            char *user = get(&table, "sender");
                            char ip[128];
                            char name[128];
                        
                            lws_get_peer_addresses(lws_get_network_wsi(wsi), lws_get_socket_fd(wsi), name, sizeof(name), ip, sizeof(ip));
                        
                            pthread_mutex_lock(&file_mutex);
                        
                            FILE *file = fopen("registries.txt", "w");  
                            if (file == NULL) {
                                sprintf(cleaned_message, "{ \"type\": \"error\", \"sender\": \"server\", \"Error\": \"Cannot open file\", \"timestamp\": \"%s\"}", timestamp);
                                perror("Error opening file");
                                pthread_mutex_unlock(&file_mutex);
                                return -1;  // Return -1 to indicate an issue
                            }
                        
                            fprintf(file, "%s,%s\n", user, ip);  // Log user disconnecting
                            fclose(file);
                        
                            pthread_mutex_unlock(&file_mutex);
                        
                            // Notify others about the disconnection
                            sprintf(cleaned_message, 
                                "{\"type\": \"user_disconnected\", \"sender\": \"%s\", \"content\": \"%s has disconnected\", \"timestamp\": \"%s\"}",
                                session->user_id, session->user_id, timestamp);
                        
                            lws_write(wsi, (unsigned char *)cleaned_message, strlen(cleaned_message), LWS_WRITE_TEXT);


                            lws_close_reason(wsi, LWS_CLOSE_STATUS_NORMAL, (unsigned char *)cleaned_message, strlen(cleaned_message));
                        
                            return -1; 
                        }
                        

                    }else{
                        sprintf(cleaned_message, "{ \"type\": \"error\", \"sender\": \"server\",\"content\": \"Error JSON FORMAT INVALID\",\"timestamp\": \"%s\"}", timestamp);
                    }
                    
                    
        
                    size_t len = strlen(cleaned_message);  // Get the length of the cleaned_message
        
                    // Allocate buffer dynamically with LWS_PRE padding
                    unsigned char *buf = malloc(LWS_PRE + len);
                    if (buf == NULL) {
                        printf("Error: Failed to allocate buffer\n");
                        return -1;
                    }
                    memcpy(&buf[LWS_PRE], cleaned_message, len);
        
                    // Send back the message
                    printf("%s, \n",cleaned_message);
                    if (lws_write(wsi, &buf[LWS_PRE], len, LWS_WRITE_TEXT) < 0) {
                        printf("Error sending message back to client\n");
                    }

                    break;
                }

            case LWS_CALLBACK_CLOSED:

                char timestamp[30];
                gettime(timestamp, sizeof(timestamp));

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (session_table[i] != NULL && session_table[i]->wsi == wsi) {
                        printf("Usuario %s desconectado (IP: %s)\n", 
                            session_table[i]->user_id, session_table[i]->ip_address);
                        
                        //Notificar desconexion
                        char notify_msg[1024];
                        snprintf(notify_msg, sizeof(notify_msg),
                                "{\"type\":\"user_disconnected\",\"sender\":\"%s\",\"content\":\"%s se ha desconectado\",\"timestamp\":\"%s\"}",
                                session_table[i]->user_id, session_table[i]->user_id, timestamp);
                        
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (session_table[j] != NULL && session_table[j]->wsi != wsi) {
                                unsigned char buf[LWS_PRE + strlen(notify_msg)];
                                memcpy(&buf[LWS_PRE], notify_msg, strlen(notify_msg));
                                lws_write(session_table[j]->wsi, &buf[LWS_PRE], strlen(notify_msg), LWS_WRITE_TEXT);
                            }
                        }
                        
                        // Limpiar la sesión
                        free(session_table[i]);
                        session_table[i] = NULL;
                        printf("Sesión liberada\n");
                        break;
                    }
                }
                break;
        }
        return 0;
    }


    

    int main(int argc, char *argv[]) {
        if (argc != 2) {
            fprintf(stderr, "Uso: %s <puertodelservidor>\n", argv[0]);
            return 1;
        }
        int port = atoi(argv[1]);

        struct lws_context_creation_info info;
        struct lws_context *context;
        struct lws_protocols protocols[] = {
            {"chat-protocol", callback_websocket, 0, 4096},
            {NULL, NULL, 0, 0}  // End of protocols
        };

        memset(&info, 0, sizeof(info));
        info.port = port; // puerto proporcionado
        info.protocols = protocols;

        context = lws_create_context(&info);
        if (!context) {
            fprintf(stderr, "Failed to create WebSocket context\n");
            return -1;
        }

        printf("WebSocket server started on port %d\n", port);

        // Event loop to process WebSocket events
        time_t last_check = time(NULL);
        while (1) {
            lws_service(context, 100); // Run event loop

            if (time(NULL) - last_check >= 30) {
                check_all_users();
                last_check = time(NULL);
            }
        }

        lws_context_destroy(context);
        return 0;
    }
