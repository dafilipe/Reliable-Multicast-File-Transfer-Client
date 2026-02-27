/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * receiver_th.h
 *
 * Header for functions that implement the receiver thread and related control
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#ifndef RECEIVERTH_H
#define RECEIVERTH_H

#include <gtk/gtk.h>
#include <netinet/in.h>

/* Symbols defined in callbakcs.h:
	MAX_MESSAGE_LEN	// Maximum length of a message

	// Packet types
	PKT_DATA - Data packet; sent by the senders
	PKT_SRR - Source Receiver Report; sent by the receivers
	PKT_STOP - Stops a transmission; sent by the senders to the receivers
	PKT_EXIT - Leave receivers group; sent by the receiver to the senders
*/

/* Parameters for file transmission defined in callback.c: */
extern const int receiver_SRR_timeout;  // Waiting time to generate SRR at the receiver
extern const int OK_timeout; // Maximum waiting time for an OK at the sender


// Receiver-thread data entry
typedef struct ReceiverTh {
	gboolean active; 			// Receiver state
	struct ReceiverTh *self; 	// Validation pointer; equal to the pointer if valid
	union {
		struct sockaddr_in addr4;	// TCP Destination address IPV4
		struct sockaddr_in6 addr;	// TCP Destination address IPV6
	}v;
	//struct sockaddr_in addr4;	// TCP Destination address IPV4
	//struct sockaddr_in6 addr;	// TCP Destination address IPV6
	//struct sockaddr_in addr4;	// TCP Destination address IPV4
	//struct sockaddr_in6 addr;	// TCP Destination address IPV6
	gboolean is_ipv4;			// TRUE if IPv4, FALSE if IPv6
	pthread_t tid; 				// Thread ID
	char fname[81]; 			// Requested filename

	int st; // TCP socket descriptor
	int sm; // Multicast UDP socket descriptor
	FILE *sf; // File descriptor
	char name_str[80];
	char name_f[256]; // name of created file
	BITMASK bmask; // BITMASK with received blocks
	gboolean saddr_def; // If sender's IP address is known
	union {
		struct sockaddr_in6 saddr6; // UDP sender's IPv6 address
		struct sockaddr_in saddr4; // UDP sender's IPv4 address
	} u;

	short int cid; 				// Client ID
	short int sid; 				// Session ID

	// Additional fields are needed to implement the receiver logic
	// ...
} ReceiverTh;



/* Function used in the main thread */
void stop_receiver_by_tid(unsigned tid, gboolean lock_gdb);		// Stop one Receiver by tid
void free_receiverTh(ReceiverTh *r, gboolean lock, gboolean lock_gdb);	// Stop one Receiver
void stop_receivers(gboolean lock_gdb);  		// Stop all receiving processes
ReceiverTh *locate_receiverTh(unsigned tid, gboolean lock);	// Locate a Receiver identified by a tid
ReceiverTh *new_receiverTh_desc(const gchar *name, const gchar *ip, int port); // Add a new receiver descriptor to the list
// Start thread for downloading a file
ReceiverTh *start_file_download(const gchar *name, const gchar *ip, int port);

/* Functions used in the file receiving threads */
// Log function for threads to display messages
void sLog(ReceiverTh *t, const char *s, gboolean togui);
// Function that sends a SRR to the sender
gboolean send_SRR(ReceiverTh *t, short int sid, short int cid);
// Function that sends a EXIT to the sender
gboolean send_EXIT(ReceiverTh *t, short int sid, short int cid);
// Function that stops a thread in a orderly way (sending EXIT message)
void sstop_thread(ReceiverTh *t, gboolean send_exit, gboolean delete_file, gboolean lock, gboolean lock_gdb);

// Function that implements the receiver algorithm; runs on a Pthread
void *receiver_thread_function(void *ptr);

#endif