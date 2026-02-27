/*****************************************************************************\
 * Redes Integradas de Telecomunicacoes
 * MIEEC/MEEC/MERSIM - FCT NOVA  2025/2026
 *
 * gui_g3.c
 *
 * Functions that handle graphical user interface interaction
 *
 * @author  Luis Bernardo
\*****************************************************************************/

#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>
#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "gui.h"
#include "file.h"
#include "callbacks.h"

// Set here the glade file name
#define GLADE_FILE "fmulticast_client.glade"

#ifdef DEBUG
#define debugstr(x)     g_print("%s", x)
#else
#define debugstr(x)
#endif


// Mutex to synchronize changes to GUI log window
pthread_mutex_t lmutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex to synchronize changes to GUI database of file transfer threads
pthread_mutex_t gmutex = PTHREAD_MUTEX_INITIALIZER;

// Counter of active threads in window
int win_cnt;

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



/** Initialization function */
gboolean
GUI_init_app (GUI_WindowElements *win)
{
        GtkBuilder              *builder;
        GError                  *err=NULL;

        /* use GtkBuilder to build our interface from the XML file */
        builder = gtk_builder_new ();
        if (gtk_builder_add_from_file (builder, GLADE_FILE, &err) == 0)
        {
                GUI_error_message (err->message);
                g_error_free (err);
                return FALSE;
        }

        /* get the widgets which will be referenced in callbacks */
        win->window = GTK_WIDGET (gtk_builder_get_object (builder,
                                                             "window1"));
        win->toggleButton1 = GTK_TOGGLE_BUTTON (gtk_builder_get_object (builder,
                											 "togglebutton1"));
        win->entryPID = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryPID"));
        win->entryFileQuery = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryFileQuery"));
        win->entryIPQuery = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryIPQuery"));
        win->entryPortQuery = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryPortQuery"));
        win->entryOutDir = GTK_ENTRY (gtk_builder_get_object (builder,
                                                             "entryOutDir"));
        win->treeProc = GTK_TREE_VIEW (gtk_builder_get_object (builder,
                                                             "treeProc"));
        win->th_store = GTK_LIST_STORE (gtk_builder_get_object (builder,
                                                             "procstore"));
        win->textView = GTK_TEXT_VIEW (gtk_builder_get_object (builder,
                                                                "textview"));
        /* connect signals, passing our TutorialTextEditor struct as user data */
        gtk_builder_connect_signals (builder, win);

        /* free memory used by GtkBuilder object */
        g_object_unref (G_OBJECT (builder));

        win_cnt= 0;

        return TRUE;
}


/** Logs the message str to the textview and command line */
void Log (const gchar * str)
{
  GtkTextBuffer *textbuf;
  GtkTextIter tend;

  // Adds text to the command line
  g_print("%s", str);

  pthread_mutex_lock( &lmutex );	// Locks log mutex
  // Write text to textArea
  textbuf = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (main_window->textView));
  gtk_text_buffer_get_iter_at_offset (textbuf, &tend, -1);	// Gets reference to the last position
  // Adds text to the textview
  gtk_text_buffer_insert (textbuf, &tend, str, strlen (str));
  pthread_mutex_unlock( &lmutex );	// unlocks log mutex
}


// Translates a string into its numerical value
static int get_number_from_text(const gchar *text) {
  int n= 0;
  char *pt;

  if ((text != NULL) && (strlen(text)>0)) {
    n= strtol(text, &pt, 10);
    return ((pt==NULL) || (*pt)) ? -1 : n;
  } else
    return -1;
}


/** Gets the PID number. Return -1 if error */
int GUI_get_PID() {
	return get_number_from_text(gtk_entry_get_text(main_window->entryPID));
}


/** Sets the PID number entry */
void GUI_set_PID(int pid) {
	char buf[10];
	sprintf(buf, "%d", pid);
	gtk_entry_set_text(main_window->entryPID, buf);
}


/** Gets the file name in the query box */
const gchar *GUI_get_QueryFile() {
	return gtk_entry_get_text(main_window->entryFileQuery);
}


/** Sets the file name in the query box */
void GUI_set_QueryFile(const char *addr) {
	gtk_entry_set_text(main_window->entryFileQuery, addr);
}


/** Gets the file name in the query box */
const gchar *GUI_get_QueryIP() {
	return gtk_entry_get_text(main_window->entryIPQuery);
}


/** Sets the file name in the query box */
void GUI_set_QueryIP(const char *addr) {
	gtk_entry_set_text(main_window->entryIPQuery, addr);
}


/** Gets the file name in the query box */
const int GUI_get_QueryPort() {
	return get_number_from_text(gtk_entry_get_text(main_window->entryPortQuery));
}


/** Sets the file name in the query box */
void GUI_set_QueryPort(unsigned int port) {
	char buf[10];
	sprintf(buf, "%u", port);
	gtk_entry_set_text(main_window->entryPortQuery, buf);
}


/** Gets the output directory's entry contents */
const gchar *GUI_get_OutDir() {
	return gtk_entry_get_text(main_window->entryOutDir);
}


/** Sets the output directory's entry contents */
void GUI_set_OutDir(const char *dir) {
	gtk_entry_set_text(main_window->entryOutDir, dir);
}


/** Blocks or unblocks the configuration GtkEntry boxes in the GUI */
void GUI_block_entrys(gboolean block)
{
  gtk_editable_set_editable(GTK_EDITABLE(main_window->entryOutDir), !block);
}



/***********************************************************\
|* Functions that handle File transfer TreeView management *|
\***********************************************************/

/** Search for 'tid' in file transfer list; returns iter */
gboolean GUI_locate_Ftrans(unsigned tid, GtkTreeIter *iter, gboolean lock_gdk) {
	assert(iter != NULL);
	GtkTreeModel *tree_model;
	gboolean valid;
	int cnt= win_cnt;

	// Get the first iter in the list
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}

	tree_model= GTK_TREE_MODEL(main_window->th_store);
	if (tree_model == NULL)
		return FALSE;
	valid = gtk_tree_model_get_iter_first (tree_model, iter);
	while (valid && (cnt-->0))
	{
		// Walk through the list, reading each row
		unsigned tid_val;

		// Make sure you terminate calls to gtk_tree_model_get() with a '-1' value
		gtk_tree_model_get (tree_model, iter,
				0, &tid_val,
				-1);
		if (tid == tid_val) {
			return gtk_list_store_iter_is_valid (GTK_LIST_STORE(main_window->th_store), iter);
		}
		valid = gtk_tree_model_iter_next (tree_model, iter);
	}
	if (lock_gdk) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}
	return FALSE;
}


/** Add line with thread 'tid' data to list */
gboolean GUI_add_Ftrans(unsigned tid, const char *ip, int port, const char *f_name, gboolean lock_gdk)
{
	assert(f_name != NULL);
	assert(ip != NULL);
	assert(tid > 0);
	assert(port > 0);
#ifdef DEBUG
	g_print("Thread %d added to list\n", tid);
#endif

	GtkTreeIter iter;
	LOCK_MUTEX(&gmutex, "lock_f1\n");
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	if (GUI_locate_Ftrans(tid, &iter, FALSE)) {
		char tmp_buf[80];
		sprintf(tmp_buf, "Replacing tid %u in table\n", tid);
		Log(tmp_buf);
	} else {
		// new file
		gtk_list_store_append(GTK_LIST_STORE(main_window->th_store), &iter);
		win_cnt++;
#ifdef DEBUG
		fprintf(stdout, "Win list cont= %d\n", win_cnt);
#endif
	}
	gtk_list_store_set(GTK_LIST_STORE(main_window->th_store), &iter, 0, tid, 1, strdup(ip),
			2, port, 3, strdup(" "), 4, strdup(f_name), -1);
	if (lock_gdk) {
		/* release GTK thread lock */
		gdk_threads_leave ();
	}

	UNLOCK_MUTEX(&gmutex, "lock_f1\n");
	return TRUE;
}


/** Update the percentage information in thread tid information */
gboolean GUI_update_Ftrans_tx(unsigned tid, int trans, int total, gboolean lock_gdk) {
	GtkTreeIter iter;
	char str_buf[80];
#ifdef DEBUG
	g_print("GUI_update_Ftrans_tx(%u - %d,%d)\n", tid, trans, total);
#endif
	LOCK_MUTEX(&gmutex, "lock_f2\n");
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	if (!GUI_locate_Ftrans(tid, &iter, FALSE)) {
		if (lock_gdk) {
			/* get GTK thread lock */
			gdk_threads_leave ();
		}
		UNLOCK_MUTEX(&gmutex, "lock_f2\n");
		return FALSE;
	}
#ifdef DEBUG
	g_print("Thread %u updated trans %d\n", tid, trans);
#endif
	sprintf(str_buf, "%d of %d", trans, total);
	gtk_list_store_set(GTK_LIST_STORE(main_window->th_store), &iter, 3, strdup(str_buf), -1);
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_leave ();
	}
	UNLOCK_MUTEX(&gmutex, "lock_f2\n");
	return TRUE;
}


/** Get all information from the line with 'tid' in the thread list */
gboolean GUI_get_Ftrans_info(unsigned tid, const char **ip, int *port,
		const char **transf, const char **filename, gboolean lock_gdk) {
	GtkTreeIter iter;
	LOCK_MUTEX(&gmutex, "lock_f3\n");
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	if (!GUI_locate_Ftrans(tid, &iter, FALSE)) {
		UNLOCK_MUTEX(&gmutex, "lock_f3\n");
		if (lock_gdk) {
			/* get GTK thread lock */
			gdk_threads_leave ();
		}
		return FALSE;
	}
	gtk_tree_model_get (GTK_TREE_MODEL(main_window->th_store), &iter, 1, ip,
			2, port, 3, transf, 4, filename, -1);
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_leave ();
	}
	UNLOCK_MUTEX(&gmutex, "lock_f3\n");
	return TRUE;
}


/** Deletes the line with 'tid' in thread list */
gboolean GUI_del_Ftrans(unsigned tid, gboolean lock_gdk) {
	GtkTreeIter iter;
#ifdef DEBUG
	g_print("GUI_del_Ftrans(%u)\n", tid);
#endif
	LOCK_MUTEX(&gmutex, "lock_f4\n");
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	if (GUI_locate_Ftrans(tid, &iter, FALSE)) {
		gtk_list_store_remove(GTK_LIST_STORE(main_window->th_store), &iter);
		win_cnt--;
#ifdef DEBUG
		g_print("Thread %u deleted from list (#=%d)\n", tid, win_cnt);
#endif
		if (lock_gdk) {
			/* get GTK thread lock */
			gdk_threads_leave ();
		}
		UNLOCK_MUTEX(&gmutex, "lock_f4\n");
		return TRUE;
	} else {
		debugstr("Failed to remove tid from list\n");
		if (lock_gdk) {
			/* get GTK thread lock */
			gdk_threads_leave ();
		}
		UNLOCK_MUTEX(&gmutex, "lock_f4\n");
		return FALSE;
	}

}


/** Deletes all threads from the thread table */
void GUI_clear_Ftrans(gboolean lock_gdk) {
	LOCK_MUTEX(&gmutex, "lock_f5\n");
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	gtk_list_store_clear(GTK_LIST_STORE(main_window->th_store));
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_leave ();
	}
	win_cnt= 0;
	UNLOCK_MUTEX(&gmutex, "unlock_f5\n");
}


/** Get selected thread id */
unsigned int GUI_get_selected_tid(gboolean lock_gdk) {
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	unsigned int tid= 0;

	LOCK_MUTEX(&gmutex, "lock_f6\n");
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_enter ();
	}
	selection = gtk_tree_view_get_selection(main_window->treeProc);
	if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
		gtk_tree_model_get(model, &iter, 0, &tid, -1);
	} else {
		if (lock_gdk) {
			/* get GTK thread lock */
			gdk_threads_leave ();
		}
		UNLOCK_MUTEX(&gmutex, "unlock_f6\n");
		Log("No file transfer is selected\n");
		return -1;
	}
	if (lock_gdk) {
		/* get GTK thread lock */
		gdk_threads_leave ();
	}
	UNLOCK_MUTEX(&gmutex, "unlock_f6\n");
	if (!tid) {
		Log("Invalid TID in the selected line\n");
		return -1;
	}
	return tid;
}



/***************************\
|*   Auxiliary functions   *|
\***************************/

/** Creates a window with an error message and outputs it to the command line */
void
GUI_error_message (const gchar *message)
{
        GtkWidget               *dialog;

        /* log to terminal window */
        g_warning ("%s", message);

        /* create an error message dialog and display modally to the user */
        dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                         GTK_MESSAGE_ERROR,
                                         GTK_BUTTONS_OK,
                                         "%s", message);

        gtk_window_set_title (GTK_WINDOW (dialog), "Error!");
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}


/** Opens a dialog window to choose a filename to read. Return the
    newly allocated filename or NULL. */
const gchar *GUI_get_open_filename (GUI_WindowElements *win)
{
        GtkWidget               *chooser;
        gchar                   *filename=NULL;

        chooser = gtk_file_chooser_dialog_new ("Open File...",
                                               GTK_WINDOW (win->window),
                                               GTK_FILE_CHOOSER_ACTION_OPEN,
                                               "_Cancel", GTK_RESPONSE_CANCEL,
                                               "_Open", GTK_RESPONSE_OK,
                                               NULL);
        if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_OK)
        {
                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
        }
        gtk_widget_destroy (chooser);
        return filename;
}


/** Opens a dialog window to choose a filename to save. Return the
    newly allocated filename or NULL. */
const gchar *GUI_get_save_filename (GUI_WindowElements *win)
{
        GtkWidget               *chooser;
        gchar                   *filename=NULL;

        chooser = gtk_file_chooser_dialog_new ("Save File...",
                                               GTK_WINDOW (win->window),
                                               GTK_FILE_CHOOSER_ACTION_SAVE,
                                               "Cancel", GTK_RESPONSE_CANCEL,
                                               "_Save", GTK_RESPONSE_OK,
                                               NULL);
        if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_OK)
        {
                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
        }
        gtk_widget_destroy (chooser);
        return filename;
}



/***********************\
|*   Event handlers    *|
\***********************/

/** Handles 'Clear' button - clears textMemo */
void on_buttonClear_clicked (GtkButton * button, gpointer user_data)
{
  GtkTextBuffer *textbuf;
  GtkTextIter tbegin, tend;

  textbuf = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (main_window->textView));
  gtk_text_buffer_get_iter_at_offset (textbuf, &tbegin, 0);
  gtk_text_buffer_get_iter_at_offset (textbuf, &tend, -1);
  gtk_text_buffer_delete (textbuf, &tbegin, &tend);
}


