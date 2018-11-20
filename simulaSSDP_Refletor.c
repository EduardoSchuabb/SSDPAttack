

#include <stdio.h>
#include <string.h>	
#include <stdlib.h>	
#include <sys/socket.h>			// Biblioteca para criacao do socket.
#include <arpa/inet.h>			// Biblioteca para uso da funcao inet_addr()
//#include <netinet/in.h>			// Biblioteca para uso da struct sockaddr_in
#include <unistd.h> 			// Biblioteca para close descritor.
#include <time.h>
#include <pthread.h>            // Biblioteca para threads.
#include <netinet/ip.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <linux/in.h>
#include <pthread.h>



static const char SSDP_IP_MULTICAST[] = "239.255.255.250";
static const int SSDP_PORT = 1900;


static const char SSDP_200_OK_MESSAGE[] = "HTTP/1.1 200 OK\r\n"\
                                        "Server: Custom/1.0 UPnP/1.0 Proc/Ver\r\n"\
                                        "EXT:\r\n"\
                                        "Location: http://fake_message:1900/test/simulaSSDP_refletor\r\n"\
                                        "Cache-Control: max-age=45\r\n"\
                                        "ST:upnp:rootdevice\r\n"\
                                        "USN: uuid: 444444-44444-44444-44444::upnp:rootdevice\r\n\r\n";

/**
 Para responder, eu preciso saber o que?
 
 Enderedo IP para onde a resposta vai
 Porta para a onde a resposta vai

*/
typedef struct respostaSimulador {
    char* ip;
    int porta;

}respostaSimulador;


void* envia_msg_200_OK_SSDP(void* arg) { 

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    respostaSimulador* structResp = (respostaSimulador*)arg;
    int count = 0;
    int my_socket;
    int returnSendto;
    struct sockaddr_in socket_out, listen_socket;

    memset(&listen_socket, 0, sizeof(listen_socket));
    listen_socket.sin_family = AF_INET;
    listen_socket.sin_port = htons(SSDP_PORT);
    //listen_socket.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_socket.sin_addr.s_addr = inet_addr("192.168.0.36");


    my_socket = socket(PF_INET, SOCK_DGRAM, 0); 
    unsigned int yes = 1;
    if( setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0){
       printf("ERROR ao reutilizar o socket\n");
    }

    if(bind(my_socket,(struct sockaddr*)&listen_socket, sizeof(listen_socket)) < 0){
        printf("ERROR in bind\n");
    }
    
    socket_out.sin_family = AF_INET; 
    //socket_out.sin_port = htons(structResp->porta); 
    socket_out.sin_port = structResp->porta; 

    socket_out.sin_addr.s_addr = inet_addr(structResp->ip);

    //Envia 3 mensagens de notify.
    sleep(1);
    while(count < 3){
    
        returnSendto = sendto(my_socket, SSDP_200_OK_MESSAGE, strlen(SSDP_200_OK_MESSAGE), 0, (struct sockaddr*)&socket_out, sizeof(struct sockaddr_in));
        printf("Retorno do sendto: %d\n", returnSendto);
        
        count++;
    }

    //exit(0); // finaliza thread.
}

int main (int argc, char** argv){

    printf("Inicia simulador dispositivo SSDP. \n");
	printf("------------------------------------------------\n");

    struct sockaddr_in listen_socket, socket_in; 
    int size_socket_in, my_socket;
    char msg_recebida[576];
    int msg_size;

    respostaSimulador* structResp = NULL;
    structResp = malloc(sizeof(respostaSimulador));

    printf("Criando socket\n");
    my_socket = socket(AF_INET, SOCK_DGRAM, 0); 
    size_socket_in = sizeof(struct sockaddr_in);

    
    unsigned int yes = 1;
    if( setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0){
       printf("ERROR ao reutilizar o socket\n");
    }

    memset(&listen_socket, 0, sizeof(listen_socket));
    listen_socket.sin_family = AF_INET;
    listen_socket.sin_port = htons(SSDP_PORT);
    //listen_socket.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_socket.sin_addr.s_addr = inet_addr(SSDP_IP_MULTICAST);

    printf("Realizando o bind socket\n");
    if(bind(my_socket,(struct sockaddr*)&listen_socket, sizeof(listen_socket)) < 0){
        printf("ERROR in bind\n");
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(SSDP_IP_MULTICAST);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    setsockopt(my_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq));

    //printf("Mandando msg teste...\n");

    //sendto(my_socket, SSDP_200_OK_MESSAGE, 512, 0, (struct sockaddr*)&listen_socket, sizeof(listen_socket));

    printf("Escutando o meio...\n");
    while(1){

        msg_size = recvfrom(my_socket, msg_recebida, 576, MSG_WAITALL, (struct sockaddr*)&socket_in, &size_socket_in);

        if(msg_size < 0){
            printf("ERROR in recvfrom\n");
        }
        else{
            //printf("Mensagem recebida:\n %s\n", msg_recebida);           

            if(strstr(msg_recebida, "M-SEARCH")){
                printf("M-SEARCH message\n");
                structResp->ip = (char*)malloc(sizeof(char)*16);  
                strcpy(structResp->ip, inet_ntoa(socket_in.sin_addr));
                structResp->porta = socket_in.sin_port;
                //printf("enviar mensagem para: %s\n",structResp->ip);
                //printf("enviar mensagem na porta htons: %d\n", htons(structResp->porta));
                //printf("enviar mensagem na porta sem htons: %d\n", structResp->porta);
                pthread_t thread_200_OK;
                pthread_create(&thread_200_OK, NULL, envia_msg_200_OK_SSDP, structResp);


            }
            if(strstr(msg_recebida, "NOTIFY")){
                //printf("NOTIFY message\n");
            }

            //printf("--------------------------------------------\n");
        }
    }
    return 0;
}


