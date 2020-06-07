#include <mictcp.h>
#include <api/mictcp_core.h>
#define FENETRE 10
#define NBRPERTES 2
/*
    Il faut vérifier que pourcentage de pertes admissibles > set_loss_rate

    |-----------------Methode fenêtre glissante, celle implémentée -------------------|
	Notre pourcentage de perte admissible correspond au rapport (nbrpertes/fenetre)*100
	La taille de la fenêtre est proportionelle à notre pourcentage de perte admissible
	plus le pourcentage de perte admissible est faible, on peut se permettre d'avoir une fenêtre 
	plus large 

    |-----------------Autre méthode, mis en commentaire-------------------|
	Decommenter la region concernée pour faire tourner l'algo avec cette méthode
*/


/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */



static int PE = 0;
static int PA = 0;
static int pertes = 0;
static int paquets = 0;
//static int perdu = 0;

mic_tcp_sock_addr socket_return;

int mic_tcp_socket(start_mode sm)
{
   int result = -1;
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   result = initialize_components(sm); /* Appel obligatoire */
   set_loss_rate(5);

   return result;
}

/*
 * Permet d’attribuer une adresse à un socket.
 * Retourne 0 si succès, et -1 en cas d’échec
 */
int mic_tcp_bind(int socket, mic_tcp_sock_addr addr)
{
   printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   return socket;
}

/*
 * Met le socket en état d'acceptation de connexions
 * Retourne 0 si succès, -1 si erreur
 */
int mic_tcp_accept(int socket, mic_tcp_sock_addr* addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
   
    unsigned long tps = 10;
    mic_tcp_sock_addr addr_recv;

    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf(" attente d'un syn\n");

    mic_tcp_pdu accept;
    accept.payload.data = NULL;
    accept.payload.size = 0;

    while(IP_recv(&accept,&addr_recv, tps) == -1 || accept.header.syn != 1);

    mic_tcp_pdu syn_ack;
    syn_ack.payload.data = NULL;
    syn_ack.payload.size = 0;
    syn_ack.header.syn = 1;
    syn_ack.header.ack = 1;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf(" reception d'un syn\n");
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf(" envoie d'un syn_ack\n");

    IP_send(syn_ack, addr_recv);

    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf(" attente d'un ack du syn_ack\n");

    mic_tcp_pdu ack_ack;
    ack_ack.payload.data = NULL;
    ack_ack.payload.size = 0;
    ack_ack.header.ack = 0;

    if (IP_recv(&ack_ack, &addr_recv, tps) != -1 || ack_ack.header.ack ==0){
        printf("ack du syn_ack recu\n");
    }
    
    printf("Sortie accept\n");
    return socket;
}

/*
 * Permet de réclamer l’établissement d’une connexion
 * Retourne 0 si la connexion est établie, et -1 en cas d’échec
 */
int mic_tcp_connect(int socket, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf("\n");
    socket_return = addr;

    mic_tcp_pdu syn;
    syn.payload.data = NULL;
    syn.payload.size = 0;
    syn.header.syn = 1;


    mic_tcp_pdu connect_ack;
    connect_ack.payload.data = NULL;
    connect_ack.payload.size = 0;
    connect_ack.header.seq_num = PE;
    unsigned long tps = 10;
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf(" envoie d'un syn\n");
    IP_send(syn, addr);
    //Attend de recevoir l'acquitemment pour la connexion avec sys et ack mises à 1
    while (IP_recv(&connect_ack, &addr, tps) == -1 || connect_ack.header.ack == 0 || connect_ack.header.syn == 0){
        IP_send(syn, addr);
    }
    
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf(" syn_ack recu\n");
    printf("[MIC-TCP] Appel de la fonction: ");  printf(__FUNCTION__); printf(" envoie ack pour syn_ack\n");

    mic_tcp_pdu ack_ack;
    ack_ack.payload.data = NULL;
    ack_ack.payload.size = 0;
    ack_ack.header.ack = 0;

    IP_send(ack_ack, addr);
    printf("Sortie connect\n");
    return socket;
}


/*
 * Permet de réclamer l’envoi d’une donnée applicative
 * Retourne la taille des données envoyées, et -1 en cas d'erreur
 */
int mic_tcp_send (int mic_sock, char* mesg, int mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_pdu pdu;
    //completer le payload
    pdu.payload.data = mesg;
    pdu.payload.size = mesg_size;


    //completer le header
    pdu.header.source_port = 0;
    pdu.header.dest_port = 0;
    pdu.header.seq_num = PE; // mis a jour de nseq avec pe
    pdu.header.ack_num = 0;
    pdu.header.syn = 0; 
    pdu.header.ack = 0; 
    pdu.header.fin = 0; 

    //remplir adresse ip
    mic_tcp_sock_addr a = socket_return;
    
    //place message dans le buffer (pas necessaire)
	//app_buffer_put(pdu.payload);

    //envoi du pdu
    IP_send(pdu, a);

    //activation du timer 
    mic_tcp_pdu pdu_recv;
    pdu_recv.payload.data = NULL;
    pdu_recv.payload.size = 0;

    mic_tcp_sock_addr addr_recv;
    unsigned long tps = 5;

    //- debut methode fenetre glissante
    //tant que ack n'est pas reçu envoi pdu
    //fiabilité partielle avec fenêtre glissante 
    //plus intéréssante pour des pertes espacés dans le temps 
    //si pertes regroupées on aurait des 'blancs' de temps en temps 
    while (IP_recv(&pdu_recv, &addr_recv, tps) == -1 || (pdu_recv.header.ack_num == PE)){
    	if (pertes < NBRPERTES){
    		pertes++;
    		break;
    	} else{
    		IP_send(pdu, a);
    	}   
    }

    paquets = (paquets + 1) % FENETRE;

    if (paquets == 0){
    	pertes = 0;
    }
    //- fin methode fenetre glissante

    //----------------------------------------------------------------------------------------------------//

    //tant que ack n'est pas reçu envoi pdu
    //fiabilité partielle calculée avec les paquets qui précèdent, 1 sur 5 peut être perdu
    //plus intéréssant pout des pertes très rapprochés 
    //- debut autre methode
    /*
    while (IP_recv(&pdu_recv, &addr_recv, tps) == -1 || (pdu_recv.header.ack_num == PE)){
        if (perdu == 0){
            perdu = (perdu + 1) % 2;
            break;
        } else {
            IP_send(pdu, a);
        }
    }

    if (pertes != 0){
        pertes = (pertes + 1) % 5;
        if (pertes == 0){
            perdu = (perdu + 1) % 2;
        }
    }

    if (pertes == 0 && perdu == 1){
        pertes = (pertes + 1) % 5;
    }
    */
    //- fin autre methode
    
    //mis a jour du PE
    PE = (PE + 1) % 2;
    return mesg_size;
}

/*
 * Permet à l’application réceptrice de réclamer la récupération d’une donnée
 * stockée dans les buffers de réception du socket
 * Retourne le nombre d’octets lu ou bien -1 en cas d’erreur
 * NB : cette fonction fait appel à la fonction app_buffer_get()
 */
int mic_tcp_recv (int socket, char* mesg, int max_mesg_size)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    mic_tcp_payload p;
    p.data = mesg;
    p.size = max_mesg_size;
    int get_size = app_buffer_get(p);
    return get_size;

}

/*
 * Permet de réclamer la destruction d’un socket.
 * Engendre la fermeture de la connexion suivant le modèle de TCP.
 * Retourne 0 si tout se passe bien et -1 en cas d'erreur
 */
int mic_tcp_close (int socket)
{
    printf("[MIC-TCP] Appel de la fonction :  "); printf(__FUNCTION__); printf("\n");
    return socket;
}

/*
 * Traitement d’un PDU MIC-TCP reçu (mise à jour des numéros de séquence
 * et d'acquittement, etc.) puis insère les données utiles du PDU dans
 * le buffer de réception du socket. Cette fonction utilise la fonction
 * app_buffer_put().
 */
void process_received_PDU(mic_tcp_pdu pdu, mic_tcp_sock_addr addr)
{
    printf("[MIC-TCP] Appel de la fonction: "); printf(__FUNCTION__); printf("\n");
    
    mic_tcp_pdu pdua;
    //completer le payload
    pdua.payload.data = NULL;
    pdua.payload.size = 0;
    pdua.header.ack = 1; 
    
    
    if (pdu.header.seq_num == PA){
        if (pdu.header.syn == 0){
            app_buffer_put(pdu.payload);
            PA = (PA + 1) % 2;
            pdua.header.ack_num = PA; 
            IP_send(pdua, addr);
        }  
    } else {
        pdua.header.ack_num = PA; 
        IP_send(pdua, addr);
    }
}
