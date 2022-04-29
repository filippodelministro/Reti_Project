#include "all.h"

//* ///////////////////////////////////////////////////////////////////////
//*                             DECLARATION                             ///
//* ///////////////////////////////////////////////////////////////////////

// -----------     DEVICE    -----------------
struct device{
    int port;                   // port number       
    int sd;                     // TCP socket
    struct sockaddr_in addr;  

    //device info
    int id;
    char* username;
    char* password;
    bool connected;         //true if chat already open
    
    //not needed for device purpouse
    ////bool registred;
    ////char time[8];

    //chat info
    int msg_pend;
}devices[MAX_DEVICES];

struct device my_device;

//-----------     SERVER    -----------------
//server considered as a device
struct device server;

//-----------     SET    -----------------
//socket which listen connect request from other devices
int listening_socket;    //socket listener (get connect request)

fd_set master;          //main set: managed with macro 
fd_set read_fds;        //read set: managed from select() 
int fdmax;

//maybe in an unic extern file utility.c            ???
//* ///////////////////////////////////////////////////////////////////////
//*                                 UTILITY                             ///
//* ///////////////////////////////////////////////////////////////////////

//prompt a boot message on stdout
void boot_message(){
    printf("**********************PEER %d**********************\n", my_device.port);
    printf( "Create an account or login to continue:\n"
                "1) signup  <srv_port> <username> <password>    --> create account\n"
                "2) in      <srv_port> <username> <password>    --> connect to server\n"
    );
}

//Function called by the server so manage socket and interaction with devices
//* ///////////////////////////////////////////////////////////////////////
//*                             FUNCTIONS                               ///
//* ///////////////////////////////////////////////////////////////////////

void list_contacts(){
    int i;
    int n_conn = 0;

    for(i=0; i<MAX_DEVICES; i++){
        if(devices[i].connected)
            n_conn++;            
    }

    printf("\n[list_device] %u devices online\n", n_conn);
    printf("\tdev_id\tusername\tport\tsocket\n");
        for(i=0; i<MAX_DEVICES; i++){

            struct device* d = &devices[i];
            if(d->connected){
                printf("\t%d\t%s\t\t%d\t%d\n",
                    d->id, d->username, 
                    d->port,
                    d->sd
                );
            }
        }
}
void fdt_init(){
    FD_ZERO(&master);
	FD_ZERO(&read_fds);
    // FD_ZERO(&write_fds);
	FD_SET(0, &master);
	
	fdmax = 0;

    printf("[device] fdt_init: set init done...\n");
}
void send_opcode(int op){
    //send opcode to server
    printf("[device] send opcode %d to server...\n", op);
    send_int(op, server.sd);
}

//*manage socket
void create_srv_socket_tcp(int p){

    // printf("[device] create_srv_tcp_socket: trying to connect to server...\n");
    server.port = p;

    //create
    if((server.sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[device] socket() error");
        exit(-1);
    }
    if(setsockopt(server.sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //address
    memset(&server.addr, 0, sizeof(server.addr));
    server.addr.sin_family = AF_INET;
    server.addr.sin_port = htons(p);
    // srv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
    inet_pton(AF_INET, "127.0.0.1", &server.addr.sin_addr);

    if(connect(server.sd, (struct sockaddr*)&server.addr, sizeof(server.addr)) == -1){
        perror("[device]: error connect(): ");
        exit(-1);
    }

    // printf("[device] create_srv_tcp_socket: waiting for connection...\n");
}
void create_listening_socket_tcp(){
    if((listening_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("[device] socket() error");
        exit(-1);
    }
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //address
    memset(&my_device.addr, 0, sizeof(my_device.addr));
    my_device.addr.sin_family = AF_INET;
    my_device.addr.sin_port = htons(my_device.port);
    my_device.addr.sin_addr.s_addr = INADDR_ANY;
    // inet_pton(AF_INET, "127.0.0.1", &server.addr.sin_addr);

    if(bind(listening_socket, (struct sockaddr*)&my_device.addr, sizeof(my_device.addr)) == -1){
        perror("[server] Error bind: \n");
        exit(-1);
    }

    listen(listening_socket, MAX_DEVICES);
    
    FD_SET(listening_socket, &master);
    if(listening_socket > fdmax){ fdmax = listening_socket; }
}
int create_chat_socket(int id, int port){

    //create socket
    if((devices[id].sd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("socket() error");
        printf("closing program...\n"); 
        exit(-1);
    }
    if(setsockopt(devices[id].sd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    //create address
    memset((void*)&devices[id].addr, 0, sizeof(devices[id].addr));
    devices[id].addr.sin_family = AF_INET;
    devices[id].addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &devices[id].addr.sin_addr);

    //connection
    if(connect(devices[id].sd, (struct sockaddr*)&devices[id].addr, sizeof(devices[id].addr)) == -1) {
        perror("connect() error");
        exit(-1);
    }

    struct device* d = &devices[id];

    // list_contacts();
    return devices[id].sd;
}

//*manage devices
void dev_init(int id, const char* usr, const char* pswd){
//initialize my_device structure with usr/pswd get by user & dev_id get server
    
    struct device* d = &my_device;

    d->id = id;
    d->username = malloc(sizeof(usr));
    d->password = malloc(sizeof(pswd));
    strcpy(d->username, usr);
    strcpy(d->password, pswd);

    printf("[device] dev_init: You are now registered!\n"
                    "\t dev_id: %u \n"
                    "\t username: %s \n"
                    "\t password: %s\n",
                    d->id, d->username, d->password
    );
}
void add_dev(int id, const char* usr, int port){
//initilize dev to communicate with
    
    struct device* d = &devices[id];
    d->username = malloc(sizeof(usr));
    strcpy(d->username, usr);
    d->port = port;

    d->connected = true;
}
int find_device_from_socket(int sd){
    int i;

    // printf("[server] find_device_from_skt: looking for %d in %d devices connected...\n", sd, n_conn);
    for(i=0; i<MAX_DEVICES; i++){
        struct device *d = &devices[i];
        
        if(d->sd == sd && d->connected)
            return i;    
    }

    return -1;      //not found
}

//*manage chats
bool check_chat_command(const char* cmd){
    if(!strncmp(cmd, "\\q", 2)){
        printf("[read_chat_command] QUIT!\n");
        return true;
    }
    else if(!strncmp(cmd, "\\a", 2)){
        printf("[read_chat_command] ADD!\n");
        //todo: add another device

        return true;
    }

    return false;
}

void handle_chat_w_server(){
    int ret, i;
    bool ok = true;
    char buffer[BUFFER_SIZE];       //buffer per memorizzare il messaggio scritto dall'utente
    
    char msg[BUFFER_SIZE];          //messaggio da inviare, formato: username [hh:mm:ss]: (msg)
    
    // Variabili per il timestamp dei messaggi
    time_t rawtime; 
    struct tm *msg_time;
    char timest[8];                 // dove metto il timestamp formattato

    // send_int(133, server.sd);

    printf("[handle_chat_w_server]\n");
    // while(ok){
    while(true){
        //keyboard: sending message
        //todo: force to remain in same line
        // printf("[%s]: ", my_device.username);
        fgets(buffer, BUFFER_SIZE, stdin);

        //sending while user type "\q"
        int code = ((check_chat_command(buffer)) ? ERR_CODE : OK_CODE);
        send_int(code, server.sd);
        if(code == ERR_CODE)
            break;

        time(&rawtime);
        msg_time = localtime(&rawtime);
        strftime(timest, 9, "%X", msg_time);
        sprintf(msg, "%s [%s]: %s", my_device.username, timest, buffer);
        
        //todo: convert in send_msg (remove BUFFER_SIZE)
        send(server.sd, msg, BUFFER_SIZE, 0);
        
    }
    
}

void handle_chat(int sock) {
    int ret, i;
    bool ok;
    char buffer[BUFFER_SIZE];       //buffer per memorizzare il messaggio scritto dall'utente
    
    char msg[BUFFER_SIZE];          //messaggio da inviare, formato: username [hh:mm:ss]: (msg)
    
    // Variabili per il timestamp dei messaggi
    time_t rawtime; 
    struct tm *msg_time;
    char timest[8];                 // dove metto il timestamp formattato

    FD_SET(sock, &master);
    fdmax = sock;
    system("clear");
    for(;;) {
        read_fds = master; 
        ret = select(fdmax + 1, &read_fds, NULL, NULL, NULL); 
		if (ret < 0) {
			perror("Errore in fase di select");
			exit(-1);
		}
		for (i = 0; i <= fdmax; i++) {
			if(FD_ISSET(i, &read_fds)) {
                if (i == 0) {                    
                    //keyboard: sending message
                    //todo: force to remain in same line
                    // printf("[%s]: ", my_device.username);
                    fgets(buffer, BUFFER_SIZE, stdin);

                    time(&rawtime);
                    msg_time = localtime(&rawtime);
                    strftime(timest, 9, "%X", msg_time);
                    sprintf(msg, "%s [%s]: %s", my_device.username, timest, buffer);
                    
                    //send in any case message: if command, inform other device
                    //todo: convert in send_msg (remove BUFFER_SIZE)
                    send(sock, msg, BUFFER_SIZE, 0);

                    if(check_chat_command(buffer)){
                        
                    }
                    else{
                        // exit(0);
                        return;
                    }
                }
                else if(i == sock){
                    // received message
                    //todo: find device who is sending message
                    // ret = find_device_from_socket(sock);
                    // printf("[%s]: ", devices[ret].username);
                    //todo: convert in recv_msg (remove BUFFER_SIZE)
                    if(recv(sock, (void*)msg, BUFFER_SIZE, 0) == 0){
                        //todo: handle this case
                        printf("Other devices quit the chat\n");
                        return;
                    }
                    

                    printf("%s", msg);
                }
            }
        }
    }
}

void handle_request(){
    printf("[handle_request]\n");

    int s_sd, s_port;
    char* s_username;
    struct sockaddr_in s_addr;
    socklen_t addrlen = sizeof(s_addr);    
    s_sd = accept(listening_socket, (struct sockaddr*)&s_addr, &addrlen);

    //receive sender info
    s_port = recv_int2(s_sd, false);
    recv_msg2(s_sd, s_username, false);

    //todo: manage history of chat

    // printf("[%s]: '%s'", s_username, b);
    //fix: ALTRO_NOME
    printf("[device] Received conncection request from 'ALTRO_NOME'\n");
    /*
    for(int i=3; i>0; i--){
        printf("[device] Chat starting in %d seconds...\r", i);
        sleep(1);
    }
    */
    
    handle_chat(s_sd);
}

//What a device user can use to interact with device
//* ///////////////////////////////////////////////////////////////////////
//*                             COMMANDS                                ///
//* ///////////////////////////////////////////////////////////////////////

//prompt an help list message on stdout
void help_command(){
	printf( "Type a command:\n"
            "1) hanging   --> receive old msg\n"
            "2) show      --> ??\n"
            "3) chat      --> ??\n"
            "4) share     --> ??\n"
            "5) out       --> ??\n"
    );
}

void signup_command(){

    char port[1024];
    char username[1024];
    char password[1024];
    char buffer[BUFFER_SIZE];

    //get data from stdin
    printf("[device] signup_command:\n[device] insert <srv_port> <username> and <password> to continue\n");

    scanf("%s", port);
    server.port = atoi(port);
    scanf("%s", username);
    scanf("%s", password);

    //prompt confermation message
    printf("[device] signup_command: got your data! \n"
        "\t srv_port: %d \n"
        "\t username: %s \n"
        "\t password: %s\n",
        server.port, username, password
    );

    //create socket to communicate with server
    create_srv_socket_tcp(server.port);

    //send opcode to server and wait for ack
    send_opcode(SIGNUP_OPCODE);
    sleep(1);

    //send username and password to server
    strcat(buffer, username);
    strcat(buffer, DELIMITER);
    strcat(buffer, password);
    send(server.sd, buffer, strlen(buffer), 0);

    //receive dev_id
    int dev_id = recv_int(server.sd);
    if(dev_id == ERR_CODE){
        printf("[device] Error in signup: username '%s' not available!\n", username);
        close(server.sd);
        exit(-1);
        return;
    }

    //update device structure with dev_id get from server
    dev_init(dev_id, username, password);

    memset(buffer, 0, sizeof(buffer));
    close(server.sd);
}

void in_command(){

    char srv_port[1024];
    char username[1024];
    char password[1024];
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    //get data from stdin
    printf("[device] in_command:\n[device] insert <srv_port> <username> and <password> to continue\n");
    scanf("%s", srv_port);
    server.port = atoi(srv_port);
    scanf("%s", username);
    scanf("%s", password);

    //prompt confermation message
    printf("[device] in_command: got your data! \n"
        "\t srv_port: %d \n"
        "\t username: %s \n"
        "\t password: %s\n",
        server.port, username, password
    );

    create_srv_socket_tcp(server.port);

    //send opcode to server and wait for ack
    send_opcode(IN_OPCODE);
    sleep(1);

    //send username and password to server
    strcat(buffer, username);
    strcat(buffer, DELIMITER);
    strcat(buffer, password);
    send(server.sd, buffer, strlen(buffer), 0);

    //send dev_id & port to server
    printf("sending dev_id: %d\n", my_device.id);
    send_int(my_device.id, server.sd);
    printf("sending port: %d\n", my_device.port);
    send_int(my_device.port, server.sd);

    //receiving ACK to connection
    if(recv_int(server.sd) == ERR_CODE){
        printf("[device] Error in authentication: check usr or pswd and retry\n");
        close(server.sd);
        return;
    }

    create_listening_socket_tcp();

    //complete: device is now online
    my_device.connected = true;
    printf("[device] You are now online!\n");
    
    memset(buffer, 0, sizeof(buffer));
    close(server.sd);
}

void hanging_command(){
	char buffer[4096];

    //first handshake
    create_srv_socket_tcp(server.port);
    send_opcode(HANGING_OPCODE);
    sleep(1);

    send_int(my_device.id, server.sd);

    prompt();
    memset(buffer, 0, sizeof(buffer));
    close(server.sd);
}

void show_command(){

    //first handshake
    create_srv_socket_tcp(server.port);
    send_opcode(SHOW_OPCODE);
    sleep(1);
    // send_int(my_device.id, server);

    printf("COMANDO SHOW ESEGUITO \n");
}

void chat_command(){
    char r_username[1024];
    int r_port, r_id, r_sd;
    scanf("%s", r_username);

    //check to avoid self-chat
    if(strcmp(r_username, my_device.username) == 0){
        perror("[device] Error: chatting with yourself");
	    return;
    } 

    //first handshake
    create_srv_socket_tcp(server.port);
    send_opcode(CHAT_OPCODE);
    sleep(1);
    send_int(my_device.id, server.sd);

    //sending chat info
    send_msg(r_username, server.sd);

    //handshake: check if registered & if online
    if(recv_int(server.sd) == ERR_CODE){
        printf("[device] user '%s' does not exists!\n", r_username);
        goto chat_end;
    }

    //receive port: chat with server if recv_device is not online
    r_port = recv_int(server.sd);
    if(r_port == server.port){
        //device is not online: chatting with server
        printf("[device] user '%s' is not online: sending messages to server!\n", r_username);

        handle_chat_w_server();   
    }
    else{
        //device is online: chatting with him
        r_id = recv_int(server.sd);
        // printf("[device] connection with '%s'\n", r_username);
        // printf("\tport:\t%d\n\tid:\t%d\n", r_port, r_id);

        //todo: check if chat already opened

        add_dev(r_id, r_username, r_port);
        r_sd = create_chat_socket(r_id, r_port);

        //sending information
        send_int(my_device.port, r_sd);
        send_msg(my_device.username, r_sd);

        /*
        printf("[device] Connected with '%s': you can now chat!\n", r_username);
        for(int i=3; i>0; i--){
            printf("[device] Chat starting in %d seconds...\r", i);
            sleep(1);
        }
        */
        handle_chat(r_sd);
    }


    chat_end:
    printf("COMANDO CHAT ESEGUITO \n");
    close(server.sd);
}

void share_command(){
    //first handshake
    create_srv_socket_tcp(server.port);
    send_opcode(SHARE_OPCODE);
    sleep(1);
    // send_int(my_device.id, server);

    printf("COMANDO SHARE ESEGUITO \n");
}

void out_command(){
    create_srv_socket_tcp(server.port);

    send_opcode(OUT_OPCODE);
    sleep(1);

    close(listening_socket);
    FD_CLR(listening_socket, &master);

    //send dev_id to server
    send_int(my_device.id, server.sd);
    
    //wait ACK from server to safe disconnect
    if(recv_int(server.sd) == my_device.id){
        my_device.connected = false;    
        printf("[device] You are now offline!\n");
    }
    else printf("[device] out_command: Error! Device not online!\n");

    close(server.sd);
}

//command for routine services
void read_command(){

    char cmd[COMMAND_LENGHT];

    //get commando from stdin
    scanf("%s", cmd);

    //todo: add clear | cls command

    //signup and in allowed only if not connected
    //other command allowed only if connected
    if(!strncmp(cmd, "help", 4)){
        if(my_device.connected)
            help_command();
        else
            boot_message();
    }
    else if(!strncmp(cmd, "signup", 6)){
        if(!my_device.connected)
            signup_command();
        else{
            printf("Device already connected! Try one of below:\n");
            help_command();
        }
    }
	else if (!strncmp(cmd, "in", 2)){
        if(!my_device.connected)
            in_command();
        else{
            printf("device already connected! Try one of below:\n");
            help_command();
        }
    }
	else if (!strncmp(cmd, "hanging", 7) && my_device.connected)	
		hanging_command();
    else if (!strncmp(cmd, "show", 4) && my_device.connected)	
		show_command();
    else if (!strncmp(cmd, "chat", 4) && my_device.connected)	
		chat_command();
	else if (!strncmp(cmd, "share", 5) && my_device.connected)	
        share_command();
    else if (!strncmp(cmd, "out", 3) && my_device.connected)
        out_command();

    //command is not valid; ask to help_command and show available command
	else{
        printf("Command is not valid!\n");
            if(my_device.connected) help_command();
            else boot_message();
    }						
}

//* ///////////////////////////////////////////////////////////////////////
//*                                 MAIN                                ///
//* ///////////////////////////////////////////////////////////////////////

//* ///////////////////////////////////////////////////////////////////////
//* ///////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
    
    int i;
    // int i, newfd, ret;
    // socklen_t addrlen;
    char buffer[BUFFER_SIZE];

    if(argc != 2){
		fprintf(stderr, "Error! Correct syntax: ./dev <port>\n"); 
		exit(-1);
    }

    my_device.port = atoi(argv[1]);

   //Initialise set structure 
	fdt_init();
    
	FD_SET(listening_socket, &master);
	fdmax = listening_socket;
    
    //prompt boot message
    boot_message();
    prompt();

    while(true){

        read_fds = master;

        if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("[device] error select() ");
			exit(-1);
		}

        for(i=0; i<=fdmax; i++){

            if(FD_ISSET(i, &read_fds)){
                
                if(i == 0){
                    //keyboard
                    read_command();
                }

                else if(i == listening_socket){
                    //connection request
                    handle_request();
                }
                
                else if(i == server.sd){
                    //connection request by server
                    // i = recv_int(server.sd);
                    // printf("[device] TEST: received %d\n", i);

                    printf("\t\ti == server.sd\n");

                }

                //clear buffer and prompt
                memset(buffer, 0, BUFFER_SIZE);
                prompt();
            }
        }
    }
}