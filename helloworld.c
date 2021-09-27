#include <stdio.h>

int main(){
	int inicio,sala,interno,abertura,externo,alarme,alarmeAcionado,arOperacao;
	printf("1- Iniciar sistema\n");
	printf("Qualquer outro botao- Fechar sistema\n");
	scanf("%d",&inicio);
	
	if(inicio == 1){
		printf("Faixa de operacao do ar\n");
		scanf("%d",&arOperacao);
		printf("sistema iniciado\n");
	}

	while(inicio == 1){	
		printf("AMBIENTE INTERNO: presenca de pessoas - 1 para sim - 2 para nao\n");
		scanf("%d",&interno);
		printf("AMBIENTE EXTERNO: presenca de pessoas - 1 para sim - 2 para nao\n");
		scanf("%d",&externo);
		printf("ABERTURA JANELA/PORTAS: - 1 aberto - 2 fechado\n");
		scanf("%d",&abertura);
		printf("AMBIENTE SALA: presenca de pessoas - 1 para sim - 2 para nao\n");
		scanf("%d",&sala);
		printf("ALARME: - 1 para ligado - 2 para desligado\n");
		scanf("%d",&alarme);
		if(interno == 1 && alarmeAcionado != 1){
			printf("AMBIENTE INTERNO: Luz acesa\n");
		}else{
			printf("AMBIENTE INTERNO: Luz apagada\n");
		}
		if(sala == 1 && alarmeAcionado != 1){
			printf("SALA: Luz acesa\n");
			//logica do ar
		}else{
			printf("SALA: Luz apagada\n");
		}	
		if(alarmeAcionado == 1){
			printf("ALARME: ligado n");
		}
	}
	return 0;
}
