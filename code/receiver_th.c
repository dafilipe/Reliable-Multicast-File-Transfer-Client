/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * receiver_th.c
 *
 * Functions that implement the receiver threads and related control
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <netdb.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include "sock.h"
#include "gui.h"
#include "bitmask.h"
#include "callbacks.h"
#include "receiver_th.h"
#include "file.h"


// Active receiver list
GList *rcv_list = NULL;
// thread static counter
static int rcv_count= 0;

// Disable DEBUG in this module
//#ifdef DEBUG
//#undef DEBUG
//#endif
//#define DEBUG 1

// Log function, for debug
void debugstr(const char *str) {
#ifdef DEBUG
	fprintf(stdout, "%s", str);
#endif
}



/****************************************\
|* Functions that handle file reception *|
 \***************************************/

// Mutex to synchronize changes to file transfer lists
pthread_mutex_t rmutex = PTHREAD_MUTEX_INITIALIZER;


#ifdef DEBUG
#define LOCK_MUTEX(mutex,str) { \
			pthread_mutex_lock( mutex ); \
			fprintf(stdout,"l %s", str); \
		}
#else
#define LOCK_MUTEX(mutex,str) { \
			pthread_mutex_lock( mutex ); \
		}
#endif

#ifdef DEBUG
#define UNLOCK_MUTEX(mutex,str) { \
			fprintf(stdout,"u %s", str); \
			pthread_mutex_unlock( mutex ); \
		}
#else
#define UNLOCK_MUTEX(mutex,str) { \
			pthread_mutex_unlock( mutex ); \
		}
#endif


/** Free the receiver data, closing everything and freeing all allocated resources */
void free_receiverTh(ReceiverTh *t, gboolean lock, gboolean lock_gdb) {
	assert(t != NULL);
	if (t->self != t) {
		debugstr("Invalid pointer in destroy_receiver\n");
		return;
	}
#ifdef DEBUG
	fprintf(stdout, "%sfree_receiverTh()\n", t->name_str);
#endif
	t->active= FALSE;
	t->self = NULL;
	if (lock)
		LOCK_MUTEX(&rmutex, "lock_r0\n");
	rcv_list = g_list_remove(rcv_list, t);
	if (lock)
		UNLOCK_MUTEX(&rmutex, "lock_r0\n");

	if (t->tid > 0) {
		GUI_del_Ftrans(t->tid, lock_gdb); // Deletes the thread entry in the window
		t->tid = 0;
	}

	if (t->st > -1) {
		close(t->st);
		t->st = -1;
	}
	if (t->sm > -1) {
		close(t->sm);
		t->sm = -1;
	}
	if (t->sf != NULL) {
		fclose(t->sf);
		t->sf = NULL;
	}
	free_bitmask(&t->bmask);

	free(t);
}


// writes to socket s and warns about errors writing
#define WARN_WRITE(s, var, size)	if (write(s,var,size)!=size) perror("Error in write to TCP socket")


/** Function that stops a thread in a orderly way (sending EXIT message, deleting the file) */
void sstop_thread(ReceiverTh *t, gboolean send_exit, gboolean delete_file, gboolean lock, gboolean lock_gdb) {
	assert(t != NULL);
	if (lock)
		LOCK_MUTEX(&rmutex, "lock_r1\n");
	if (t->self != t) {
		debugstr("Invalid pointer in destroy_receiver\n");
		if (lock)
			UNLOCK_MUTEX(&rmutex, "lock_r1\n");
		return;
	}

#ifdef DEBUG
	fprintf(stdout, "%s sstop_thread(%d,%d)\n", t->name_str, send_exit, delete_file);
#endif
	// Anticipates flag and window changes
	t->active= FALSE;
	if (t->tid > 0) {
		GUI_del_Ftrans(t->tid, lock_gdb); // Deletes the thread entry in the window
		t->tid = 0;
	}

	if (send_exit) {
		send_EXIT(t, t->sid, t->cid);
	}
	if (t->st > -1) {
		if (send_exit)
			WARN_WRITE(t->st, "END", 4);
	}
	if (t->sf != NULL) {
		fclose(t->sf);
		if (delete_file) {
			unlink(t->name_f);
		}
		t->sf = NULL;
	}
	free_receiverTh(t, FALSE, lock_gdb);
	if (lock)
		UNLOCK_MUTEX(&rmutex, "lock_r1\n");
}


/** Stop one Receiver by tid */
void stop_receiver_by_tid(unsigned tid, gboolean lock_gdk) {
	ReceiverTh *r= locate_receiverTh(tid, lock_gdk);
	if (r != NULL) {
#ifdef DEBUG
		fprintf(stdout, "receiver(%u) stopped\n", tid);
#endif
		sstop_thread(r, TRUE, TRUE, TRUE, lock_gdk);
	} else {
		debugstr("Invalid tid in stop_receiver_by_tid\n");
	}
}


/** Stop all receiving processes */
void stop_receivers(gboolean lock_gdb) {
#ifdef DEBUG
		fprintf(stdout, "stop_receivers()\n");
#endif
	LOCK_MUTEX(&rmutex, "lock_r1b\n");
	while (rcv_list != NULL) {
		assert(rcv_list->data != NULL);
		ReceiverTh *r= (ReceiverTh *) rcv_list->data;
		sstop_thread(r, TRUE, TRUE, FALSE, lock_gdb);
	}
	UNLOCK_MUTEX(&rmutex, "lock_r1b\n");
}


/** Locate a Receiver identified by a pid */
ReceiverTh *locate_receiverTh(unsigned tid, gboolean lock) {
	LOCK_MUTEX(&rmutex, "lock_r2a\n");
	GList *l = rcv_list;
	for (l = rcv_list; l != NULL; l = g_list_next(l)) {
		ReceiverTh *pt = (ReceiverTh *) l->data;
		if (pt->self != pt) {
			debugstr("Invalid pointer in locate_receiver\n");
			continue;
		}
		if (((unsigned)pt->tid) == tid) {
			UNLOCK_MUTEX(&rmutex, "lock_r2a\n");
			return pt;
		}
	}
	UNLOCK_MUTEX(&rmutex, "lock_r2a\n");
	return NULL;
}


/** Add a new receiver descriptor to the list */
ReceiverTh *new_receiverTh_desc(const gchar *name, const gchar *ip, int port) {
	assert((name!=NULL) && (ip!=NULL) && (port>0));

	struct in6_addr srv_addr;
	struct hostent *hp;
	gboolean is_ipv4;

	// Translate IP address to IPv6 equivalent address
	if ((hp = gethostbyname2(ip, AF_INET6)) != NULL) {
		// It is an IPv6 address
		bcopy(hp->h_addr, &srv_addr, 16);
		is_ipv4= FALSE;
	} else if ((hp = gethostbyname2(ip, AF_INET)) != NULL) {
		// It is an IPv4 address

		// TASK x
		// Apply dual stack and the address format ::ffff:IPv4 to connect to the IPv4 server
		// Use the function 'translate_ipv4_to_ipv6' to implement this function
		// ...

		// Cria variavel para dar translate
		//struct in4_addr addr4;

		// Se der para dar translate ele da e muda guarda na stack
		/*if(translate_ipv4_to_ipv6(ip,addr4 )){
			bcopy(hp->h_addr, &srv_addr, 16);
		}*/

		if(translate_ipv4_to_ipv6(ip, /*Mandar pointer da struct*/&srv_addr)){
			printf("SUCESSO NO IPV4 PARA IPV6\n");
			//bcopy(hp->h_addr, &srv_addr, 16);
			is_ipv4= TRUE;
		}
		else{
			printf("ERROR IPV4 TO IPV6");
		}


		//Log("new_receiverTh_desc incomplete - IPv4 not supported yet\n");
	} else {
		sprintf(tmp_buf, "Unknown destination '%s'\n", ip);
		Log(tmp_buf);
		return NULL;
	}

	ReceiverTh *r = (ReceiverTh *) malloc(sizeof(ReceiverTh));

	strncpy(r->fname, name, sizeof(r->fname)-1);
	// struct sin6_addr structure to connect to the server

	if(is_ipv4){
		r->is_ipv4= TRUE;
		r->v.addr4.sin_family = AF_INET;
		r->v.addr4.sin_port = htons(port);
		bcopy(&srv_addr, &r->v.addr4.sin_addr, hp->h_length);
		//memset(r->v.addr4.sin_zero, 0, sizeof(r->v.addr4.sin_zero));

	}
	else{
		r->v.addr.sin6_family = AF_INET6;
		bcopy(&srv_addr, &r->v.addr.sin6_addr, hp->h_length);
		r->v.addr.sin6_port = htons(port);
		r->v.addr.sin6_flowinfo = 0;
		r->is_ipv4 = FALSE;

	}


	r->tid = 0;
	r->st = -1; // TCP socket descriptor
	r->sm = -1; // Multicast UDP socket descriptor
	r->sf = NULL; // File descriptor
	r->saddr_def = FALSE; // If sender's IP address is known

	r->cid = -1; // Client ID
	r->sid = -1; // Session ID
	new_empty_bitmask(&r->bmask);

	// r->DATA_cnt = 0; // Received DATA packet's counter since last SRR
	// r->SRR_cnt = 0; // SRR sent counter since last packet

	r->self = r;		// self-pointer, to validate receiver descriptor
	r->active = FALSE;

	rcv_list = g_list_append(rcv_list, r);
	return r;
}




/** Log function for thread that send messages using the pipe */
void sLog(ReceiverTh *t, const char *s, gboolean togui) {
	char stmp_buf[300];
	assert(s != NULL);
	assert((strlen(t->name_str)<=12) && (strlen(t->name_str)>7));
	assert(strlen(s)<sizeof(stmp_buf));
	sprintf(stmp_buf, "%s%s\n", t->name_str, s);
	if (togui) {
		// get GTK thread lock
		gdk_threads_enter ();
		Log(stmp_buf);
		// release GTK thread lock
		gdk_threads_leave ();
	} else
		fprintf(stdout, "%s", stmp_buf);
}


/** Function that sends a SRR to the sender */
gboolean send_SRR(ReceiverTh *t, short int sid, short int cid) {
	if (!t->saddr_def) {
		debugstr("FLAG saddr_def is FALSE\n");
		return FALSE;
	}
	char buf[2000], *pt = buf;
	char type = PKT_SRR;

	assert(!bitmask_isempty(&t->bmask));
	WRITE_BUF(pt, &type, sizeof(char));
	WRITE_BUF(pt, &t->sid, sizeof(t->sid));
	WRITE_BUF(pt, &t->cid, sizeof(t->cid));
	WRITE_BUF(pt, t->bmask.mask, t->bmask.B_len);
	int n= 0;
	if (t->is_ipv4) {
		// IPv4
		// TASK x - implement the missing code

		// Criar struct do addr4
		//struct sockaddr_in addr4;

		// Vai copiar os valores do porto e do adreco praa a struct
		//addr4.sin_port = t->u.saddr4.sin_port;
		//addr4.sin_addr = t->u.saddr4.sin_addr;


		//n = sendto(t->sm, buf, pt - buf, 0, (struct sockaddr *)&addr4, sizeof(addr4));
		//Log("IPv4 not supported on send_SRR\n");
		//return FALSE;
		struct sockaddr_in addr4;
		//memset(&addr4, 0, sizeof(addr4));

		addr4.sin_family = AF_INET;
		addr4.sin_port = t->u.saddr4.sin_port;
		addr4.sin_addr = t->u.saddr4.sin_addr;

		n = sendto(t->sm, buf, pt - buf, 0, (struct sockaddr *)&addr4, sizeof(addr4));




	} else {
		// IPv6
		n= sendto(t->sm, buf, pt - buf, 0, (struct sockaddr *) &t->u.saddr6,
				sizeof(struct sockaddr_in6));
	}

	if (n != (pt - buf)){
		perror("RCV>sendto(SRR)");
	}
	else {
		if (t->is_ipv4) {
			// IPv4
			// TASK x - implement the missing code
			// ...
			sprintf(tmp_buf, "Sent SRR(SID=%hd,CID=%hd,M=%s) to %s-%d", t->sid, t->cid, bitmask_to_string(&t->bmask), addr_ipv4(&t->u.saddr4.sin_addr), (int) ntohs(t->u.saddr4.sin_port));
			sLog(t, tmp_buf, FALSE);

		} else {
			// IPv6
			sprintf(tmp_buf, "Sent SRR(SID=%hd,CID=%hd,M=%s) to %s-%d", t->sid, t->cid, bitmask_to_string(&t->bmask), addr_ipv6(&t->u.saddr6.sin6_addr), (int) ntohs(t->u.saddr6.sin6_port));
			sLog(t, tmp_buf, FALSE);
		}
	}
	printf("O n E IGUAL A %d", n);
	return (n == (pt - buf));
}


/** Function that sends a SRR to the sender */
gboolean send_EXIT(ReceiverTh *t, short int sid, short int cid) {
	if (!t->saddr_def || (t->sm < -1))
		return FALSE;
	char buf[10], *pt = buf;
	char type = PKT_EXIT;

	WRITE_BUF(pt, &type, sizeof(char));
	WRITE_BUF(pt, &t->sid, sizeof(t->sid));
	WRITE_BUF(pt, &t->cid, sizeof(t->cid));
	int n= 0;
	if (t->is_ipv4) {
		// IPv4
		// TASK x - implement the missing code

		// Criar struct do addr4
		struct sockaddr_in addr4;

		// Vai copiar os valores do porto e do adreco praa a struct
		addr4.sin_port = t->u.saddr4.sin_port;
		addr4.sin_addr = t->u.saddr4.sin_addr;

		n = sendto(t->sm, buf, pt - buf, 0, (struct sockaddr *)&addr4, sizeof(addr4));
		//Log("IPv4 not supported on send_SRR\n");
		return FALSE;
		//Log("IPv4 not supported on send_EXIT\n");
		return FALSE;
		//
	} else {
		// IPv6
		n= sendto(t->sm, buf, pt - buf, 0, (struct sockaddr *) &t->u.saddr6,
				sizeof(struct sockaddr_in6));

	}
	if (n != (pt - buf))
		perror("RCV>sendto(EXIT)");
	else {
		if (t->is_ipv4) {
			// IPv4
			// TASK x - implement the missing code
			// ...
			sprintf(tmp_buf, "Sent EXIT(SID=%hd,CID=%hd,M=%s) to %s-%d", t->sid, t->cid,
					bitmask_to_string(&t->bmask), inet_ntoa(t->u.saddr4.sin_addr),
					(int) ntohs(t->u.saddr4.sin_port));
			sLog(t, tmp_buf, FALSE);
		} else {
			// IPv6
			sprintf(tmp_buf, "Sent EXIT(SID=%hd,CID=%hd) to %s-%d", t->sid, t->cid,
					addr_ipv6(&t->u.saddr6.sin6_addr), (int) ntohs(t->u.saddr6.sin6_port));
			sLog(t, tmp_buf, FALSE);
		}
	}
	return (n == (pt - buf));
}


// Auxiliary macro that stops a thread and frees the descriptor
#define STOP_THREAD(pt, send_exit, delete_file) {if (pt->self == pt) \
							    sstop_thread(pt, send_exit, delete_file, TRUE, TRUE); \
							return NULL; \
						}


/**
 * Thread function that implements the receiver algorithm;
 * 		runs in an independent threads
 **/
void *receiver_thread_function(void *ptr) {
	int block_size, n_blocks;
	unsigned long long f_length;
	unsigned int f_hash;
	struct ipv6_mreq imr_MCast6; // To regist socket in the IPv6 multicast group address
	struct ip_mreq imr_MCast4; // To regist socket in the IPv4 multicast group address
	u_short MCast_port;
	static char buf[8000];
	static char stmp_buf[8000];
	struct timeval tv1, tv2, timeout; // To measure the transfer duration
	struct timezone tz;
	// ...

	ReceiverTh *t= (ReceiverTh *)ptr;
	assert(t != NULL);

	// Add to the GUI thread list
	GUI_add_Ftrans((unsigned)t->tid, addr_ipv6(&t->v.addr.sin6_addr), ntohs(t->v.addr.sin6_port), t->fname, TRUE);
	usleep(1000);  // Give some time for the thread to appear in the graphical interface

	// Thread code
	sprintf(t->name_str, "RCV(%d)> ", ++rcv_count);
	fprintf(stdout, "%sstarted reading thread (file= '%s' tid = %u)\n",
			t->name_str, t->fname, (unsigned) t->tid);
	if (gettimeofday(&tv1, &tz)) {
		perror("getting reception starting time");
	}

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Create a TCP socket to send requests
	// t->st = ...
	// Cria SOcket ipv4
	if (t->is_ipv4){
		//t->st =  socket (AF_INET, SOCK_STREAM/*SOCK_DGRAM*/, 0);
		t->st = init_socket_ipv4(SOCK_STREAM, 0, 0);
	}
	// Cria Socket ipv6
	else{
		//t->st = socket (AF_INET6, SOCK_STREAM/*SOCK_DGRAM*/, 0);
		t->st = init_socket_ipv6(SOCK_STREAM, 0, 0);
	}



	t->active = TRUE;

	fprintf(stdout, "RCV> Conneting to %s - %hu\n",
			addr_ipv6(&t->v.addr.sin6_addr), ntohs(t->v.addr.sin6_port));

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Connect to the server
	/*
	    ...
	    if (connect(t->st, ...) < 0) {
			perror("RCV>error connecting TCP socket to request file");
			sLog(t, "connection failed", TRUE);
			STOP_THREAD(t, FALSE, FALSE);
		}
	*/
	if(t->is_ipv4){
		// FAZ a conexao IPV4 ao servidor, caso nao fizer cria o erro ; Cool
		if (connect(t->st,(struct sockaddr *) &t->v.addr4, sizeof (t->v.addr4)) < 0) {
					perror("RCV>error connecting TCP socket to request file");
					sLog(t, "connection failed", TRUE);
					STOP_THREAD(t, FALSE, FALSE);
		}
	}
	else{
		// FAZ a conexao IPV6 ao servidor, caso nao fizer cria o erro ; Cool
		if (connect(t->st,(struct sockaddr *) &t->v.addr, sizeof (t->v.addr)) < 0) {
					perror("RCV>error connecting TCP socket to request file");
					sLog(t, "connection failed", TRUE);
					STOP_THREAD(t, FALSE, FALSE);
		}
	}



	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Send the request to the server using the TCP connection
	/*
	    ...
		if (send(t->st, ... , 0) < 0) {
			perror("RCV>error sending request");
			sLog(t, "failed to send name", TRUE);
			STOP_THREAD(t, FALSE, FALSE);
		}
	*/

	/*if (send(t->st,t->name_f, sizeof(t->name_f), 0) < 0) {
				perror("RCV>error sending request");
				sLog(t, "failed to send name", TRUE);
				STOP_THREAD(t, FALSE, FALSE);
	}*/
	if (send(t->st, t->fname, strlen(t->fname) + 1, 0) < 0) {
	    perror("RCV>error sending request");
	    sLog(t, "failed to send name", TRUE);
	    STOP_THREAD(t, FALSE, FALSE);
	}





	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Configure the TCP socket (t->st) to wait of a maximum of OK_timeout miliseconds
	// for an answer from the server
	// ...
	// TUDO COPIADO DO ficheiro serv2v6.c
	//struct timeval timeout;

	/* Configura socket para esperar no máximo 10 segundos */
	timeout.tv_sec= OK_timeout * 10;	//METER OK_timeout em Segundos
	timeout.tv_usec= 0; //uSegundos
	if (setsockopt(t->st, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) <0) {
		perror ("Erro a definir timeout");
		exit (1);
	}







	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Receive the reply header:

	// Read the CID
	if (recv(t->st, &t->cid, sizeof(t->cid), 0) <= 0) {
		if (errno == EWOULDBLOCK) {
			sLog(t, "Timeout waiting for a server's response in TCP", TRUE);
		} else {
			perror("RCV>did not receive a response");
			sLog(t, "did not receive a response", TRUE);
		}
		STOP_THREAD(t, FALSE, FALSE);
	}


	// Test if file does not exist and read and display the error message before stopping the thread
	// if (t->cid < 0) {
	//     ...
	// }
	if(t->cid < 0){
		perror("ERRO NO CID");
	}

	// Read the others fields of the reply (use timeout OK):
	//	...
	// 		short int SID; 				// Session ID, to t->sid
	//	...
	//		unsigned long long f_length; 	// File length
	//	...
	//		int block_size;				// Block size
	//	...
	//		int n_blocks; 				// Number of blocks to receive, to n_blocks
	//	...
	//		unsigned int f_hash; 		// File hash value
	//	...
	//  	if (t->is_ipv4) {
	//			// Read the IPv4 multicast address
	//			struct in_addr maddr4;		// multicast IPv4 address to receive the file
	//			...
	//  	} else {
	//			// Read the IPv4 multicast address
	//			struct in6_addr maddr;		// multicast IPv6 address to receive the file
	//			...
	//  	}
	//
	//		unsigned short int MCast_port;	// multicast port to receive the file
	//	...

	//	#ifdef DEBUG
	//		fprintf(stdout, "%s (CID=%hd,SID=%hd,BL_S=%d,N_BL=%d,F_LEN=%llu,HASH=%u)\n",
	//				t->name_str, t->cid, t->sid, block_size, n_blocks, f_length, f_hash);
	//	#endif

    // RECEBER O SID
	if (recv(t->st, &t->sid, sizeof(t->sid), 0) <= 0) {
		if (errno == EWOULDBLOCK) {
			sLog(t, "Erro no SID", TRUE);
		} else {
			perror("SID > ERROR");
			sLog(t, "SID", TRUE);
		}
		STOP_THREAD(t, FALSE, FALSE);
	}

	// RECEBER O F_LENGTH
	if (recv(t->st, &f_length, sizeof(f_length), 0) <= 0) {
		if (errno == EWOULDBLOCK) {
			sLog(t, "Erro no F_LENGTH", TRUE);
		} else {
			perror("F_LENGTH > ERROR");
			sLog(t, "F_LENGTH", TRUE);
		}
		STOP_THREAD(t, FALSE, FALSE);
	}

	// RECEBER O BLOCK_SIZE
	if (recv(t->st, &block_size, sizeof(block_size), 0) <= 0) {
		if (errno == EWOULDBLOCK) {
			sLog(t, "Erro no BLOCK_SIZE", TRUE);
		} else {
			perror("BLOCK_SIZE > ERROR");
			sLog(t, "BLOCK_SIZE", TRUE);
		}
		STOP_THREAD(t, FALSE, FALSE);
	}

	// RECEBER O N_BLOCKS
	if (recv(t->st, &n_blocks, sizeof(n_blocks), 0) <= 0) {
		if (errno == EWOULDBLOCK) {
			sLog(t, "Erro no N_BLOCKS", TRUE);
		} else {
			perror("N_BLOCKS > ERROR");
			sLog(t, "N_BLOCKS", TRUE);
		}
		STOP_THREAD(t, FALSE, FALSE);
	}

	// RECEBER O F_HASH
	if (recv(t->st, &f_hash, sizeof(f_hash), 0) <= 0) {
		if (errno == EWOULDBLOCK) {
			sLog(t, "Erro no F_HASH", TRUE);
		} else {
			perror("F_HASH > ERROR");
			sLog(t, "F_HASH", TRUE);
		}
		STOP_THREAD(t, FALSE, FALSE);

	}


	// Read the IP multicast address
		struct in_addr maddr4;		// multicast IPv4 address to receive the file
		struct in6_addr maddr6;		// multicast IPv6 address to receive the file
	// RECEBER O ENDEREÇO MULTICAST IPV4 OU IPV6
	if (t->is_ipv4) {
		if (recv(t->st, &maddr4, sizeof(maddr4), 0) <= 0) {
			if (errno == EWOULDBLOCK) {
				sLog(t, "Erro no MCAST IPV4", TRUE);
			} else {
				perror("MCAST IPV4 > ERROR");
				sLog(t, "MCAST IPV4", TRUE);
			}
			STOP_THREAD(t, FALSE, FALSE);
		}
	} else {

		if (recv(t->st, &maddr6, sizeof(maddr6), 0) <= 0) {
			if (errno == EWOULDBLOCK) {
				sLog(t, "Erro no MCAST IPV6", TRUE);
			} else {
				perror("MCAST IPV6 > ERROR");
				sLog(t, "MCAST IPV6", TRUE);
			}
			STOP_THREAD(t, FALSE, FALSE);
		}
	}

	// RECEBER O MCAST_PORT
	if (recv(t->st, &MCast_port, sizeof(MCast_port), 0) <= 0) {
		if (errno == EWOULDBLOCK) {
			sLog(t, "Erro no MCAST_PORT", TRUE);
		} else {
			perror("MCAST_PORT > ERROR");
			sLog(t, "MCAST_PORT", TRUE);
		}
		STOP_THREAD(t, FALSE, FALSE);
	}






	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Prepare UDP socket to receive multicast traffic

	// Create a new UDP socket to receive the multicast messages using a shared port
	// if (t->is_ipv4) {
	//		// Prepare IPv4 UDP socket to receive multicast traffic
	// 		t->sm = ...
	//
	// 		Join the IPv4 multicast group
	// 		...
	//	} else {
	//		// Prepare IPv6 UDP socket to receive multicast traffic
	// 		t->sm = ...
	//
	// 		Join the IPv6 multicast group
	// 		...
	//	}
	if(t->is_ipv4){

		// Create SOCKET
		t->sm = init_socket_ipv4(SOCK_DGRAM, MCast_port, TRUE);
    	// EXIST OR NOT
		if (t->sm < 0) {
        	perror("RCV>init_socket_ipv4()");
        	sLog(t, "failed to create IPv4 UDP socket", TRUE);
        	STOP_THREAD(t, TRUE, TRUE);
    	}
		struct ip_mreq imr_MCast4;
		memset(&imr_MCast4, 0, sizeof(imr_MCast4));

		imr_MCast4.imr_multiaddr = maddr4;      // Endereço multicast recebido do servidor
		imr_MCast4.imr_interface.s_addr = htonl(INADDR_ANY); // Usa interface default

		if (setsockopt(t->sm, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					   (char *)&imr_MCast4, sizeof(imr_MCast4)) < 0) {
			perror("RCV>setsockopt(IP_ADD_MEMBERSHIP)");
			sLog(t, "failed to join IPv4 multicast group", TRUE);
			STOP_THREAD(t, TRUE, TRUE);
		}
		}
	else{
		// Create SOCKET
		t->sm = init_socket_ipv6(SOCK_DGRAM, MCast_port, TRUE);
    	// EXIST OR NOT
		if (t->sm < 0) {
        	perror("RCV>init_socket_ipv6()");
        	sLog(t, "failed to create IPv6 UDP socket", TRUE);
        	STOP_THREAD(t, TRUE, TRUE);
    	}
		struct ipv6_mreq imr_MCast6;
		memset(&imr_MCast6, 0, sizeof(imr_MCast6));

		imr_MCast6.ipv6mr_multiaddr = maddr6;
		imr_MCast6.ipv6mr_interface = 0; // 0 → interface default

		if (setsockopt(t->sm, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
		               (char *)&imr_MCast6, sizeof(imr_MCast6)) < 0) {
		    perror("RCV>setsockopt(IPV6_ADD_MEMBERSHIP)");
		    sLog(t, "failed to join IPv6 multicast group", TRUE);
		    STOP_THREAD(t, TRUE, TRUE);
		}

		sLog(t, "Joined IPv6 multicast group successfully", FALSE);



	}





	printf("numero de blocos recebidos ---->>>>>>>%d\n\n\n", n_blocks);
	// Initialize the bitmask
	new_bitmask(&t->bmask, n_blocks);

	// Create a file where the data will be stored
	if (strlen(path_dir) > 0) {
		fprintf(stdout, "path_dir='%s' name='%s'\n", path_dir, t->fname);
		// Define the filename as "path/"tid".Name"
		snprintf(t->name_f, sizeof(t->name_f), "%s/%d.%s", path_dir, (unsigned)t->tid, t->fname);
	} else {
		// Define the filename as "Name"."pid"
		snprintf(t->name_f, sizeof(t->name_f), "%d.%s", (unsigned)t->tid, t->fname);
	}
	fprintf(stdout, "%sWill store data in file '%s'\n", t->name_str, t->name_f);

	// Open file for writing
	if ((t->sf = fopen(t->name_f, "w")) == NULL) {
		perror("RCV>failed to open file");
		sLog(t, "failed to open file", TRUE);
		STOP_THREAD(t, TRUE, FALSE);
	}


	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Send "OK" answer to the server using the TCP socket and write or send
	// ...

	// Send "OK" confirmation to the server (TCP)
	const char *ok_msg = "OK";
	if (send(t->st, ok_msg, strlen(ok_msg), 0) < 0) {
		perror("RCV>error sending OK");
		sLog(t, "failed to send OK", TRUE);
		STOP_THREAD(t, TRUE, TRUE);
	}

	sLog(t, "ENVIOU O OK", FALSE);



	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Prepare structures to read multicast data, with a timeout of 'receiver_SRR_timeout'
	// Suggestion: Study how to use the select function together with the fd_set masks ...
	int smask_size = max(t->st, t->sm)+1;	// select mask size
	int n;
	// fd_set ...
	// ...
	fd_set read_fds;
	struct timeval sel_timeout;
	// Initializations
	FD_ZERO(&read_fds);
	FD_SET(t->sm, &read_fds); // multicast socket
	//FD_SET(t->st, &read_fds); // TCP socket
	sel_timeout.tv_sec = receiver_SRR_timeout;
	sel_timeout.tv_usec = 0;

	//n = select(smask_size, &read_fds, NULL, NULL, &sel_timeout);

	/*
	if (n < 0) {
		perror ("Interruption of select"); // errno = EINTR foi interrompido
	}
	else if (n == 0) {
		fprintf(stderr, "Timeout\n"); …
	} else {
	if (FD_ISSET(sd1, &rmask)) {// Há dados disponíveis para leitura em sd1

	}
	if (FD_ISSET(sd2, &rmask)) { // Há dados disponíveis para leitura em sd2

	}
	}*/


#ifdef DEBUG
	fprintf(stdout, "RCV> Main cycle (Timeout=%d s)\n",
			receiver_SRR_timeout / 1000);
#endif

	// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	// Data reading loop

	int data_counter = 0;
	do {
		// Initializations
		// ... initialize the fd_set and timeout values ...

		if (t->st >= 0) FD_SET(t->st, &read_fds);

		sel_timeout.tv_sec = receiver_SRR_timeout;
		sel_timeout.tv_usec = 0;
		int smask_size = max(t->st, t->sm)+1;


		// Wait for multicast or TCP packets, up to SRR_Timeout seconds
		// n= 0; // n= select(smask_size, ...);  -  n must have the value returned by select function
		n = select(smask_size, &read_fds, NULL, NULL, &sel_timeout);

		if (n == -1) {
			// ---------------------------------------------------------------
			if (errno == EINTR) {
				sLog(t, "select interrupted by signal", FALSE);
				continue;
			} else
				perror("RCV>select");
			// Error - stop thread
			sLog(t, "select failed", TRUE);
			STOP_THREAD(t, TRUE, TRUE);
		} else if (n == 0) {
			// ---------------------------------------------------------------
			// Timeout
			// ...
			// Send SRR message
			if (!bitmask_isempty(&t->bmask)) {
				sLog(t, "Timeout expired - sending SRR", FALSE);
				if (!send_SRR(t, t->sid, t->cid)) {
					sLog(t, "failed to send SRR", TRUE);
					STOP_THREAD(t, TRUE, TRUE);
				}
			}

		} else {
			// ---------------------------------------------------------------
			// Received a data packet
			if (FD_ISSET(t->st, &read_fds)){
				printf("Server Error, shut down connection \n");
				STOP_THREAD(t, TRUE, TRUE);

			}
			if(FD_ISSET(t->sm, &read_fds)){
				// Define auxiliary variables ...
				char *pt = buf;
				struct sockaddr_in addrec;
				struct sockaddr_in6 addrec6;
				unsigned char type;
				short int sid;
				int seq;
				int len;

/*
				if(t->is_ipv4){ // IPV4
					socklen_t addrlen = sizeof(addrec);
					n = recvfrom(t->sm, buf, sizeof(buf), 0, (struct sockaddr *)&addrec, &addrlen);
				}
				else { // IPV6
					socklen_t addrlen6 = sizeof(addrec6);
					n = recvfrom(t->sm, buf, sizeof(buf), 0, (struct sockaddr *)&addrec6, &addrlen6);

				}

				if(n <= 0) {
					perror("RCV>recvfrom");
					sLog(t, "Error reading UDP data", TRUE);
					STOP_THREAD(t, TRUE, TRUE);
				}

*/
				if (t->is_ipv4) {
					struct sockaddr_in addrec;
					socklen_t addrlen = sizeof(addrec);
					n = recvfrom(t->sm, buf, sizeof(buf), 0,
								 (struct sockaddr *)&addrec, &addrlen);
					if (n <= 0) { perror("recvfrom"); STOP_THREAD(t, TRUE, TRUE); }
					if (!t->saddr_def) { t->u.saddr4 = addrec; t->saddr_def = TRUE; }
				} else {
					struct sockaddr_in6 addrec6;
					socklen_t addrlen6 = sizeof(addrec6);
					n = recvfrom(t->sm, buf, sizeof(buf), 0,
								 (struct sockaddr *)&addrec6, &addrlen6);
					if (n <= 0) { perror("recvfrom"); STOP_THREAD(t, TRUE, TRUE); }
					if (!t->saddr_def) { t->u.saddr6 = addrec6; t->saddr_def = TRUE; }
				}

				READ_BUF(pt, &type, sizeof(type));
				printf("RCV> RECEBEU type=%d\n", type);
				// depois continua com o processamento do bloco

				/*switch(type) {
					case 1:
						READ_BUF(pt, &sid, sizeof(sid));
						READ_BUF(pt, &seq, sizeof(seq));
						READ_BUF(pt, &len, sizeof(len));
						printf("RCV> Received packet: type=%d, sid=%d, seq=%d, len=%ld\n", type, sid, seq, len);
						// VALIDAR LEN SID SEQ E DATA
				}*/


				switch(type) {
					case 1:
						// DATA TYPE
						READ_BUF(pt, &sid, sizeof(sid));
						READ_BUF(pt, &seq, sizeof(seq));
						READ_BUF(pt, &len, sizeof(len));
						printf("RCV> Received packet: type=%d, sid=%d, seq=%d, len=%ld\n", type, sid, seq, len);
						// VALIDAR LEN SID SEQ E DATA
						break;
					case 2:
						// SRR DO NOTHING
						break;
					case 3:
						// STOP TYPE
						READ_BUF(pt, &sid, sizeof(sid));
						printf("RCV> STOP: type=%d, sid=%d\n", type, sid);
						STOP_THREAD(t, TRUE, TRUE);
						break;
					case 4:
						// EXIT TYPE
						// Do NOTHING
						break;


				}

					if (seq < 0 || seq >= t->bmask.b_len) {
						char tmp[200];
						sprintf(tmp, "RCV> Invalid block sequence %d (b_len=%d)", seq, t->bmask.b_len);
						send_SRR(t, t->sid, t->cid);
						sLog(t, tmp, TRUE);
						continue; // ou STOP_THREAD(t, TRUE, TRUE) se for fatal
					}

					//int adjusted_seq = seq - 1;
					if(/*TESTAR SE A DATA E NOVA OU NAO*/ bit_isset(&t->bmask,seq) == 0){
						// WRITE FILE
						data_counter++;
						long offset = (long) seq * len;
						if (fseek(t->sf, offset, SEEK_SET) != 0) {
							perror("RCV>fseek");
							sLog(t, "Error positioning file pointer", TRUE);
							STOP_THREAD(t, TRUE, TRUE);
						}
						fwrite(pt, 1, len, t->sf);
						GUI_update_Ftrans_tx((unsigned)t->tid, count_bits(&t->bmask), t->bmask.b_len, TRUE);
						set_bit(&t->bmask,seq); // ATUALIZAR BITMASK
					}
					//		Do not forget to send SRR for every 2 DATA packets or at the end of the file
					if (data_counter % 2 == 0 ) {
				        send_SRR(t, t->sid, t->cid);
				        printf("RCV> SRR SEND\n");
				    }
					if(all_bits(&t->bmask)){
						// FECHAR FICHEIRO
						printf("RCV> ALL BLOCKS RECEIVED\n");
						fclose(t->sf);
						t->sf = NULL;
						GUI_update_Ftrans_tx((unsigned)t->tid, count_bits(&t->bmask), t->bmask.b_len, TRUE);
						STOP_THREAD(t, TRUE, TRUE);
					}
			}
			/*else{
				printf("ERRO NO SERVER\n");
				fclose(t->sf);
				t->sf = NULL;
				STOP_THREAD(t, TRUE, TRUE);
			}*/

		}
	}

	while (active && (t->self == t) && t->active
			/* && put here the other conditions to continue in the loop */);
	STOP_THREAD(t, TRUE, TRUE);
			return NULL;	// ends the thread
}


/** Start thread for downloading a file */
ReceiverTh *start_file_download(const gchar *name, const gchar *ip, int port) {
	if ((name == NULL) || (ip == NULL) || (port <= 0)) {
		debugstr("Error: null param in start_file_download\n");
		return NULL;
	}

	ReceiverTh *t = new_receiverTh_desc(name, ip, port);

	// Start the thread
	if (pthread_create(&t->tid, NULL, receiver_thread_function, (void *)t)) {
		fprintf(stderr, "main: error starting thread\n");
		free (t);
		return NULL;
	}

	return t;
}