#include <mictcp.h>
#include <api/mictcp_core.h>

/*
 * Permet de créer un socket entre l’application et MIC-TCP
 * Retourne le descripteur du socket ou bien -1 en cas d'erreur
 */
 static int PE = 0;
 static int PA = 0;

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

    int size_sent;

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
    size_sent = IP_send(pdu, a);

    //activation du timer 
    mic_tcp_pdu pdu_recv;
    pdu_recv.payload.data = NULL;
    pdu_recv.payload.size = 0;

    mic_tcp_sock_addr addr_recv;
    unsigned long tps = 5;

    //tant que ack n'est pas reçu envoi pdu
    while (IP_recv(&pdu_recv, &addr_recv, tps) == -1 || pdu_recv.header.ack != 1){
        //printf("[MIC-TCP perte packet]: " ); printf(__FUNCTION__); printf("\n");
        size_sent = IP_send(pdu, a);
    }
    printf("[MIC-TCP reception d'un ack]: " ); printf(__FUNCTION__); printf("\n");
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

    //completer le header
    pdua.header.source_port = 0;
    pdua.header.dest_port = 0;
    pdua.header.seq_num = 0; 
    pdua.header.ack_num = 0;
    pdua.header.syn = 0; 
    pdua.header.ack = 1; 

    if (pdu.header.seq_num == PA){
        app_buffer_put(pdu.payload);
        printf("[MIC-TCP envoie d'un ack]: " ); printf(__FUNCTION__); printf("\n");
        PA = (PA + 1) % 2;
    } else {
        pdua.header.ack = 0;
    }
    IP_send(pdua, addr);
    printf("[MIC-TCP] envoi d'un ACK "); printf(__FUNCTION__); printf("\n");


}
