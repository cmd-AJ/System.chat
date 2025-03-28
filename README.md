# System.chat


BY AJ and ABBY


### Libreria Libwebsocket
libwebsockets es una biblioteca C para implementar servidores y clientes WebSocket eficientes. Permite gestionar conexiones WebSocket en tiempo real mediante el uso de eventos, y es adecuada para aplicaciones de alto rendimiento. La biblioteca permite definir protocolos personalizados y manejar eventos como la recepción de mensajes o la desconexión de clientes mediante callbacks. En un entorno multihilo, libwebsockets puede trabajar con múltiples hilos, y se debe utilizar mutexes para proteger el acceso a las estructuras compartidas. 

To run Server use the command to compile 

Server
---
    gcc server.c hash.c -o server_program -lwebsockets 

Client
---
    gcc client.c -o client -lwebsockets 


## Run executables
    Para correr el programa servidor
---
    ./server_program   
---
    ./client

---
## Server Functions


callback_chat() //Se encarga de manejar las llamadas y las salidas del cliente al server


LWS_CALLBACK_ESTABLISHED: Hace el primer contacto con el cliente y el server.

LWS_CALLBACK_RECEIVE: Recibe información en string de tipo JSON 
Se parsea el JSON a un HASH y primero consigue el tipo y para cada tipo se parsea los componentes indicados de cada tipo.

En la siguiente parte se explica que hace si identifica el tipo que se registra para realizar un callback


register:
Se consigue primero el nombre y la IP de la persona
Revisa el usuario y IP de la persona 
Lo registra el usuario y la IP se escriben en un archivo txt utilizando mutex para que pueda leer cada linea sin datos dirty
Leyendo cada linea revisa si esta o no el usuario ya registrado o no.
        
Si encuentra uno da un response de error
Si no, se desbloquea el mutex y se desbloquea el mutex para empezar a agregar el usuario y la IP para asi ya desbloquear el mutex luego se imprime el formato JSON.

    
private: 
Se lee el valor de target del hash para conseguir el usuario objetivo y el contenido de mensaje

Por cada sesion que hay se busca el nombre del usuario y se envia un json con el contenido del mensaje al usuario objetivo.


broadcast: 
Se lee el valor de target del hash para conseguir el usuario objetivo y el contenido de mensaje.
Por cada sesion que hay se envia el json.


list_users: 
Se muestra todos los usuarios del server


user_info:
Consigue dentro del HASH un usuario objetivo.
Leemos los archivo y por cada linea. revisar si encuentra el usuario y conseguimos la IP y el nombre del usuario


change_status:
Cambio dentro de la sesión el estatus a Activo a Inactivo o a IDLE


Disconnect:
Se bloquea el arhivo para podes escribir a todos los usuarios que estan conectados. El que se va a desconectar no se escribe en el archivo, una vez realizado esto se debloquea el mutex. 

    
## Funciones Extras dentro de hash.c

hash(const char *key)
Crea un objeto HASH donde guardamos el valor y la llave tipo JSON


create_entry(const char *key, const char *value)
Crea una tabla para almacenar objeto tipo HASH donde guardamos a todos los datos del JSON

Viene Insert y Get para conseguir e insertar los HASH a la tabla.


char *trim(char *str) 
Solo quita el espaciado que hay en los datos 


clean_json_message:
Al traer el JSON solo quitamos los componentes que no son necesarios como las llaves



gettime(char *buffer, size_t buffer_size)  
    Consigue el tiempo en formato timestamp




Cliente:

1. Definiciones y Variables Globales

    MAX_MSG_LEN: Tamaño máximo de un mensaje (512 bytes).

    MAX_CLIENTS: Número máximo de clientes admitidos (100).

    struct lws *web_socket: Puntero al WebSocket activo.

    const char *username: Nombre de usuario del cliente.

    int running: Variable de control para mantener el programa en ejecución.

2. Función message_send

Envía mensajes JSON al servidor WebSocket según el tipo de acción:

    register: Registra al usuario en el servidor.

    broadcast: Envía un mensaje público a todos los usuarios.

    private: Envía un mensaje privado a un usuario específico.

    list_users: Solicita la lista de usuarios conectados.

    user_info: Solicita información de un usuario específico.

    change_status: Cambia el estado del usuario (ACTIVO, OCUPADO, INACTIVO).

    disconnect: Notifica la desconexión del usuario.

3. Función chat_callback (Manejador de eventos WebSocket)

Controla los eventos del WebSocket:

    LWS_CALLBACK_CLIENT_ESTABLISHED: Se activa cuando la conexión con el servidor se establece correctamente.

    LWS_CALLBACK_CLIENT_RECEIVE: Procesa mensajes recibidos del servidor (JSON).

    LWS_CALLBACK_CLIENT_WRITEABLE: Se activa cuando el cliente puede enviar datos.

    LWS_CALLBACK_CLOSED: Se activa cuando la conexión con el servidor se cierra.

4. Función show_help

Muestra los comandos disponibles en el chat:

/help               → Muestra la ayuda  
/list               → Lista los usuarios conectados  
/info <usuario>     → Muestra información de un usuario  
/status <ESTADO>    → Cambia tu estado (ACTIVO, OCUPADO, INACTIVO)  
/msg <usuario> <msg> → Envía un mensaje privado  
/exit               → Sale del programa  

5. Función receive_messages 

Ejecuta un bucle que mantiene la conexión WebSocket activa y procesa eventos entrantes mediante lws_service.
6. main() 

Inicializa y valida los parámetros de entrada:

    username → Nombre del usuario.

    server_ip → Dirección IP del servidor WebSocket.

    server_port → Puerto del servidor.

Configura el contexto WebSocket con lws_create_context.

Establece la conexión WebSocket con lws_client_connect_via_info.

Lanza un hilo (pthread_create) para recibir mensajes de forma asíncrona.

Maneja la entrada del usuario y ejecuta los comandos según corresponda.

Finaliza la ejecución cerrando la conexión WebSocket y destruyendo el contexto.

Flujo de Ejecución

El usuario inicia el programa con:

    ./cliente_chat usuario1 192.168.1.100 9000

El cliente se conecta al servidor y se registra automáticamente.

Se muestra el menú de comandos y el usuario puede interactuar en el chat.

El programa mantiene la conexión hasta que el usuario escribe /exit.