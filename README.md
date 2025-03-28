# System.chat


BY AJ and ABBY


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

En la siguiente parte se explica que hace si identifica el tipo 

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
Se lee el valor de target del hash para conseguir el usuario objetivo y el contenido de mensaje

Por cada sesion que hay se envia el json.


list_users: 
Se muestra todos los usuarios del server


user_info:
Consigue dentro del HASH un usuario objetivo
Leemos los archivo y por cada linea. revisar si encuentra el usuario y conseguimos la IP y el nombre del usuario


change_status:
Cambio dentro de la sesión el estatus a Activo a Inactivo o a IDLE


Disconnect:
Se bloquea el arhivo para podes escribir a todos los usuarios que estan conectados. El que se va a desconectar no se escribe en el archivo, una vez realizado esto se debloquea el mutex. 

    
Funciones Extras dentro de hash.c

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