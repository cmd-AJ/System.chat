#include <stdio.h>
#include <stdlib.h>
#include <libwebsockets.h>
#include <string.h>
#include <ncurses.h>

#define MAX_MSG_LEN 256   // tamaño mensaje

//ventana ?
WINDOW *win_chat, *win_user, *win_input;

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
    wprintw(win_chat, "%s/n", message);
    wrefresh(win_chat);
}

//CONN SERVER

static int chat_callback(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len){
    switch (reason){
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            message_text("CONEXION AL SERVIDOR");
            break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            message_text((char *)in);
            break;
        case LWS_CALLBACK_CLOSED:
            message_text("FIN CONEXION");
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
    lws_write(web_socket, (unsigned char *)message, strlen(message), LWS_WRITE_TEXT);
}

// comando ayuda
void help(){
    werase(win_chat);
    box(win_chat, 0,0);
    
}