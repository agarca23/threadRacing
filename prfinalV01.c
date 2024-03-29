#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/syscall.h>

int aleatorio (int minimo, int maximo);

void nuevoCorredor();

void *accionesCorredor(void *numCorredor);

void *accionesBox(void *numBox);

void *accionesJuez();

void writeLog (char id[50], char msg[50], int num1, int num2);

void final();

//Declaración de recursos

pthread_mutex_t semaforoCorredor;
pthread_mutex_t semaforoLog;
pthread_mutex_t semaforoBox1;
pthread_mutex_t semaforoBox2;
pthread_mutex_t semaforoJuez;
pthread_cond_t juez;
int numCorredores;
struct listaCorredores{
	int id;
	int estado;//estado=1 no necesita entrar en box
	int correr;//estado=1 puede correr
	int sancionado;
	int tiempoTotal;
	int terminado;
};
struct listaCorredores corredores[5];
struct listaBox{
	int atendidos;
	int parado;
};
struct listaBox boxes[2];
FILE *fichero;

int main(){
	srand (time(NULL));

//Iniciamos las señales, los recursos y los semáforos

	if(signal(SIGUSR1, nuevoCorredor)==SIG_ERR){
		perror("Error en la llamada a signal");
		exit(-1);
	}

	if(signal(SIGTERM, final)==SIG_ERR){
		perror("Error en la llamada a signal");
		exit(-1);
	}

	if(pthread_mutex_init(&semaforoCorredor, NULL)!=0){
		perror("Error en la creación del semáforo");
		exit(-1);
	}

	if(pthread_mutex_init(&semaforoBox1, NULL)!=0){
		perror("Error en la creación del semáforo");
		exit(-1);
	}

	if(pthread_mutex_init(&semaforoBox2, NULL)!=0){
		perror("Error en la creación del semáforo");
		exit(-1);
	}
	
	if(pthread_mutex_init(&semaforoJuez, NULL)!=0){
		perror("Error en la creación del semáforo");
		exit(-1);
	}

	if(pthread_mutex_init(&semaforoLog, NULL)!=0){
		perror("Error en la creación del semáforo");
		exit(-1);
	}

	//variable de condicion

	if (pthread_cond_init(&juez, NULL)!=0){
		perror("Error en la variable condicion.");
		exit(-1);
	}

	//Resto de variables

	numCorredores = 0;
	fichero = fopen("registroCarrera.log", "w");
	int i;
	for(i=0; i<5; i++){
		corredores[i].id=0;
		corredores[i].estado=1;
		corredores[i].correr= 1;
		corredores[i].sancionado=0;
		corredores[i].tiempoTotal=0;
		corredores[i].terminado=0;
	}
	
	//crear hilos e iniciarlos

	pthread_t box1, box2;
	int b1=1, b2=2;
	
	pthread_create (&box1, NULL, accionesBox, (void*)&b1);
	pthread_create (&box2, NULL, accionesBox, (void*)&b2);
	
	pthread_t juez;
	pthread_create (&juez, NULL, accionesJuez, NULL);

	for(i=0; i<2;i++){
		boxes[i].atendidos=0;
		boxes[i].parado=0;
	}
	
	//Esperamos hasta que llegue una señal

	while(1){
		pause();
	}
}


void nuevoCorredor(){
	int i;
	if(signal(SIGUSR1, nuevoCorredor)==SIG_ERR){
		perror("Error en la llamada a signal");
		exit(-1);
	}

	//Buscamos un lugar vacío y creamos un nuevo corredor
	
	pthread_mutex_lock(&semaforoCorredor);
	for(i=0; i<5; i++){
		if(corredores[i].id==0){
			numCorredores++;
			corredores[i].id=numCorredores;
			corredores[i].estado=1;
			corredores[i].correr=1;
			corredores[i].sancionado=0;
			corredores[i].tiempoTotal=0;
			corredores[i].terminado=0;

			//creamos el hilo por cada nuevo corredor

			pthread_t corredor;
			pthread_create(&corredor, NULL, accionesCorredor, (void*)&i);
			break;
		}
		else{}//poner mensaje de que el circuito ya esta lleno
	}
	pthread_mutex_unlock(&semaforoCorredor);
}

void *accionesCorredor(void *numCorredor){
	
	//variables
	int n = *(int*)numCorredor;
	int identificadorCorredor=corredores[n].id;
	int j=0;
	int tiempoVuelta;
	int numVuelta=0;
	int problemas;
	char id[50]= "Corredor_";
	char msg1[50]= "empieza la vuelta";
	char msg2[50]= "termina la vuelta con un tiempo de:";
	char msg3[50]= "entra en el box1";
	char msg4[50]= "entra en el box2";
	char msg5[50]= "abandona la carrera";
	char msg6[50]= "ha terminado la carrera";
	char msg7[50]= "empieza la carrera";
	char msg8[50]= "es sancionado";
	char msg9[50]= "ha cumplido la sancion";

	//registramos el inicio de la vuelta del corredor

	writeLog(id, msg7, identificadorCorredor, 0);

	//comienza a correr si no tiene problemas
	while(1){
		pthread_mutex_lock(&semaforoCorredor);
		if(corredores[*(int*)numCorredor].correr==1){	
			pthread_mutex_unlock(&semaforoCorredor);
			while(numVuelta!=20){
				pthread_mutex_lock(&semaforoCorredor);
				if(corredores[*(int*)numCorredor].sancionado==1){
					pthread_mutex_unlock(&semaforoCorredor);
					writeLog(id, msg8, identificadorCorredor, 0);
					pthread_mutex_lock(&semaforoJuez);
                                	pthread_cond_signal(&juez);
                             		pthread_cond_wait(&juez, &semaforoJuez);
                              		writeLog(id, msg9, identificadorCorredor, 0);
                             		pthread_mutex_unlock(&semaforoJuez);	
					tiempoVuelta=aleatorio(2,5);
					writeLog(id, msg1, identificadorCorredor, 0);
					sleep(tiempoVuelta);
					tiempoVuelta=tiempoVuelta+3;
				}else{
					tiempoVuelta=aleatorio(2,5);
					writeLog(id, msg1, identificadorCorredor, 0);
					sleep(tiempoVuelta);
				}
				corredores[n].tiempoTotal=corredores[n].tiempoTotal+ tiempoVuelta;
				pthread_mutex_unlock(&semaforoCorredor);
				problemas = aleatorio(0,1);
				if(problemas==1){
					pthread_mutex_lock(&semaforoCorredor);
					corredores[*(int*)numCorredor].estado=0;
					pthread_mutex_unlock(&semaforoCorredor);
				}
				writeLog(id, msg2, identificadorCorredor, tiempoVuelta);
				numVuelta ++;
			}
			if(numVuelta==20){
				writeLog(id, msg6, identificadorCorredor, 0);
				exit(-1);
			}
		}else if(corredores[*(int*)numCorredor].correr==0){
			pthread_mutex_lock(&semaforoCorredor);
			writeLog(id, msg5, identificadorCorredor, 0);
			for(j=n; j<4; j++){
				corredores[j].id=corredores[j+1].id;
                                corredores[j].estado=corredores[j+1].estado;
				corredores[j].correr=corredores[j+1].correr;
				corredores[j].sancionado=corredores[j+1].sancionado;
			}
			corredores[j+1].id=0;
                        corredores[j+1].estado=1;
			corredores[j+1].correr=1;
			corredores[j+1].sancionado=0;
			pthread_mutex_unlock(&semaforoCorredor);
			pthread_exit(NULL);
		}
	}
}

void *accionesBox(void *numBox){
	int i;
	int problemasGraves;
	int tiempoBox;
	int num=*(int*)numBox;
	char box[50]="Box_";
	char msg1[50]="Atiende a corredor_";
	char msg2[50]="Stop para reabastecerse";
	char msg3[50]="vuelve a estar activo";

	//esperan a que lleguen corredores para atenderlos
	while(1){
		sleep(1);
		for(i=1; i<6; i++){
			pthread_mutex_lock(&semaforoCorredor);
			if(corredores[i].id!=0 && corredores[i].estado==0){
				//se atiende al corredor
				corredores[i].estado=1;
				pthread_mutex_unlock(&semaforoCorredor);
				writeLog(box, msg1, num, corredores[i].id);
				sleep(aleatorio(1,3));
				boxes[num].atendidos++;
				problemasGraves = aleatorio(1,10);

				//comprobar si un corredor no puede continuar con la carrera

				if(problemasGraves>7){
					pthread_mutex_lock(&semaforoCorredor);
					corredores[i].correr=0;
					pthread_mutex_unlock(&semaforoCorredor);
				}
			
				//comprobar si algun box tiene que parar

				if((boxes[num].atendidos>=3) && (boxes[1].parado==0) && (boxes[2].parado==0)){
					writeLog(box, msg2, num, 0);
					boxes[num].parado=1;
					sleep(20);
					boxes[num].parado=0;
					boxes[num].atendidos=0;
					writeLog(box, msg3, num, 0);
				}
			}
			pthread_mutex_unlock(&semaforoCorredor);
		}
	}
}

void *accionesJuez(){
	int a=0;
	while(1){
		sleep(10);
		if(numCorredores!=0){
			//elegimos corredor a sancionar, si no está corriendo buscamos otro.
			a=aleatorio(1,numCorredores);
			while(corredores[a].correr==1){
				a=aleatorio(1,numCorredores);
			}
			pthread_mutex_lock(&semaforoCorredor);
			corredores[a].sancionado=1;
			pthread_cond_wait(&juez, &semaforoJuez);
			sleep(3);
			corredores[a].sancionado=0;//después de cumplir la sanción		
			pthread_mutex_unlock(&semaforoCorredor);
			pthread_cond_signal(&juez);
		}
	}
}

//Funcion para escribir en el log
void writeLog(char id[50], char msg[50], int num, int num2) {
	pthread_mutex_lock(&semaforoLog);

	// Calculamos la hora actual
	time_t now = time(0);
	struct tm *tlocal = localtime(&now);
	char stnow[19];
	strftime(stnow, 19, "%d/%m/%y %H:%M:%S", tlocal);

	// Escribimos en el log
	if (num2==0){
		fichero = fopen("registroCarrera.log", "a");
		fprintf(fichero, "[%s] %s%d: %s\n", stnow, id, num, msg);
		fclose(fichero);
	} else {
		fichero = fopen("registroCarrera.log", "a");
                fprintf(fichero, "[%s] %s%d: %s%d\n", stnow, id, num, msg, num2);
                fclose(fichero);
        
        }
	
	
	pthread_mutex_unlock(&semaforoLog);
}



//hara un resumen de la carrera??
void final(){
	//si nos da tiempo crearemos un listado de como han quedado todos los corredores
	int i;
	int tiempoGanador=0;
	int ganador;
	char id[50]="Ganador_";
	char msg1[50]="Tiempo_total :";
	for(i=0; i=numCorredores; i++){
		pthread_mutex_lock(&semaforoCorredor);
		if(corredores[i].terminado==1){
			if(corredores[i].tiempoTotal<=tiempoGanador){
				tiempoGanador=corredores[i].tiempoTotal;
				ganador=corredores[i].id;

			}
		}
		pthread_mutex_unlock(&semaforoCorredor);
	}
	writeLog(id, msg1, ganador, tiempoGanador);
	exit(0);
}


//Funcion que genera un numero aleatorio
int aleatorio(int minimo, int maximo){
	return rand() % (maximo-minimo+1) + minimo;
}
