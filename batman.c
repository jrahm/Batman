#include <gtk/gtk.h>
#include <pthread.h>

#include <string.h>
#include "batstr.h"
#include <unistd.h>

#define ICON_DIR "/usr/share/batman/icons/"

#define CHARGING_ICON "battery_charging.png" 
#define FULL_ICON     "battery_full.png" 
#define HIGH_ICON     "battery_high.png" 
#define HALF_ICON     "battery_half.png" 
#define LOW_ICON      "battery_low.png" 
#define EMPTY_ICON    "battery_empty.png" 

enum PowerState {
	  PowerState_Charging
	, PowerState_Full
	, PowerState_High
	, PowerState_Half
	, PowerState_Low
	, PowerState_Empty
};

const char * PowerStateStrings[] = {
	  ICON_DIR CHARGING_ICON
	, ICON_DIR FULL_ICON
	, ICON_DIR HIGH_ICON
	, ICON_DIR HALF_ICON
	, ICON_DIR LOW_ICON
	, ICON_DIR EMPTY_ICON
};

struct MainApp {
	struct PowerSupply_Session* session;
	GtkStatusIcon* icon;

	char tooltip_buffer[1024];
	enum PowerState power_state;

	pthread_t main_thread;
	pthread_mutex_t mutex;
};

struct MainApp* main_app = NULL;

void MainApp_UpdateSession ( struct MainApp* app ) ;

/**
 *  What is called when an adapter is plugged in
 */
void Adapter_Callback( struct PowerSupply* ths,
	struct PowerSupplyEvent* event ) {

	struct MainApp* app = (struct MainApp*) ths->udef;

	if( app ) {
		MainApp_UpdateSession( app );
	}
}

char* _voltage_markup( double volts, char* buffer, size_t size ) {
    snprintf(buffer, size, "<span foreground=\"#00FF00\">%.02fV</span>", volts);
    return buffer;
}

char* _power_markup( double watts, char* buffer, size_t size ) {
    snprintf(buffer, size, "<span foreground=\"#00FF00\">%.02fW</span>", watts);
    return buffer;
}

gboolean _main_app_update_session_idle( gpointer _app ) {
    struct MainApp* app = _app;
	pthread_mutex_lock( & app->mutex );

	PowerSupplySession_GetDeviceList( app->session );
	struct PowerSupply* dev;
	enum   PowerState state;
	double remaining;

	size_t total_energy_now = 0;
	size_t total_energy = 0;

	size_t nadapters = 0;
	size_t nbatteries = 0;
	size_t nbytes = 0;
	size_t size = 1024;

	char* next = app->tooltip_buffer;
    nbytes = snprintf(next, size, "<span font-family=\"monospace\">");
	size -= nbytes;
	next += nbytes;

    char buf[256];
    char buf2[256];

	/* Set the state to the lowest possible */
	app->power_state = PowerState_Empty;

	PowerSupplySession_ForeachDevice( app->session, dev ) {
		if( dev->type == PowerSupplyType_Adapter ) {
            /* The device is an adapter. Create the string to reflect 
             * as such */
			nbytes = snprintf( next, size, "Adapter %lu: %s\n\n", (unsigned long)nadapters ++,
				dev->adapter.online ? "Plugged In" : "Unplugged" );

			size -= nbytes;
			next += nbytes;

			if( dev->adapter.online ) {
                /* Whether or not the adapter is online or not */
				app->power_state = PowerState_Charging;
			}

            /* Attach a callback to it */
			dev->EventDetected = Adapter_Callback;
			dev->udef = app;
		} else if ( dev->type == PowerSupplyType_Battery ) {
			remaining = Battery_FractionFull( &dev->battery );

            /* Big snprintf */
			nbytes = snprintf( next, size, "Battery %lu: %.02f%%\n"
				"   %s %s\n    Status  : %s\n    Voltage : %s\n    Power   : %s\n\n"
				, (unsigned long) nbatteries ++, remaining * 100,
				dev->battery.manufacturer,
				dev->battery.technology,
				dev->battery.status,
				_voltage_markup(dev->battery.voltage / 1000000.0, buf, sizeof (buf)),
				_power_markup(dev->battery.power_now / 1000000.0, buf2, sizeof (buf2))
                );

			total_energy_now += dev->battery.energy_now;
			total_energy     += dev->battery.energy_full;

			size -= nbytes;
			next += nbytes;
			
		}
	}

	remaining = (double)total_energy_now / total_energy ;

	snprintf( next, size, "Total Remaining: %.2f%% </span>", remaining * 100 );


    GdkPixbuf* image = draw_pixbuf(remaining, 96, 96, app->power_state == PowerState_Charging);
    gtk_status_icon_set_from_pixbuf(app->icon, image);
    g_object_unref(image);

	gtk_status_icon_set_tooltip_markup
		( app->icon, app->tooltip_buffer );

	pthread_mutex_unlock( & app->mutex );

    return 0;
}

void MainApp_UpdateSession ( struct MainApp* app ) {
    gdk_threads_add_idle( _main_app_update_session_idle, app );
}

void* main_thread( void* __a ) {
	struct MainApp* app = (struct MainApp*)__a;

    while ( 1 ) {
    	MainApp_UpdateSession( app );
        sleep( 10 );
    }
};

void OnClick( GtkWidget* obj, GdkEventButton * event, gpointer data ) {
	printf( "data: %p\n", data );

	if( fork () ) {
		MainApp_UpdateSession( (struct MainApp*) data );
	} else {
		execl("/usr/bin/gnome-power-statistics", 
			"gnome-power-statistics", NULL);
		perror( "Execl failed");
	}
}

int main( int argc, char** argv ) {
	struct MainApp app;

	pthread_mutex_init( & app.mutex, NULL );
	gtk_init(&argc, &argv);

	app.session = PowerSupplySession_New( ) ;
	app.icon = gtk_status_icon_new( );

	printf( "app: %p\n", &app );
	g_signal_connect(G_OBJECT(app.icon), "button-press-event", 
	      G_CALLBACK(OnClick), &app);

	PowerSupplySession_StartMonitoring( app.session );

	pthread_create( &app.main_thread, NULL, main_thread, &app );
	gtk_main();

	return 0;
}
