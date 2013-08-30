#include "batstr.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

#define POWER_SUPPLY_SUBSYSTEM_STR "power_supply"
#define BATTERY_TYPE_STR "Battery"
#define ADAPTER_TYPE_STR "Mains"

#define DEBUG( fmt, ... ) printf( fmt, ##__VA_ARGS__ )

/*
 * Create a new session. This connects to the 
 * udev and sets up monitoring */
struct PowerSupply_Session* PowerSupplySession_New( ) {
	struct PowerSupply_Session* ret = calloc( 1, sizeof( struct PowerSupply_Session ) );

	pthread_mutex_init( & ret->m_mutex, NULL );
	if( ! (ret->udev = udev_new()) ) {
		free( ret );
		return NULL;
	}

	return ret;
}

void PowerSupply_Free( struct PowerSupply *power_supply ) {
	udev_device_unref( power_supply->udev );
	free( power_supply->dev_path );

	if( power_supply->type == PowerSupplyType_Battery ) {
		free( power_supply->battery.manufacturer );
		free( power_supply->battery.technology );
		free( power_supply->battery.status );
	}
}

void Create_Battery( struct PowerSupply* dev ) {
	const char* tmp ;

	tmp = udev_device_get_sysattr_value( dev->udev, "energy_now" );
	dev->battery.energy_now = tmp ? strtol( tmp, NULL, 10 ) : -1 ;

	tmp = udev_device_get_sysattr_value( dev->udev, "energy_full" );
	dev->battery.energy_full = tmp ? strtol( tmp, NULL, 10 ) : -1 ;

	tmp = udev_device_get_sysattr_value( dev->udev, "manufacturer" );
	dev->battery.manufacturer = strdup( tmp );

	tmp = udev_device_get_sysattr_value( dev->udev, "power_now" );
	dev->battery.power_now = tmp ? strtol( tmp, NULL, 10 ) : -1 ;

	tmp = udev_device_get_sysattr_value( dev->udev, "technology" );
	dev->battery.technology = strdup( tmp );

	tmp = udev_device_get_sysattr_value( dev->udev, "voltage_now" );
	dev->battery.voltage = tmp ? strtol( tmp, NULL, 10 ) : -1 ;

	tmp = udev_device_get_sysattr_value( dev->udev, "status" );
	dev->battery.status = strdup( tmp );
}

/*
 * Populates the session with new devices and
 * starts the monitoring thread */
void PowerSupplySession_GetDeviceList( struct PowerSupply_Session* ses ) {
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	
	const char* tmp;

	size_t i = 0;
	pthread_mutex_lock( & ses->m_mutex ) ;

	if( ses->ndevices > 0 ) {
		/* If there exist devices, well delete them
		 * and refresh the list */
		for( i = 0 ; i < ses->ndevices; ++ i ) {
			PowerSupply_Free( &ses->devices[i] ) ;
		}
		ses->ndevices = 0;
		free( ses->devices );
		ses->devices = NULL;
		i = 0;
	}

	/*
	 * Get the list of devices in the subsystem 'power_supply'
	 */
	enumerate = udev_enumerate_new( ses->udev );
	udev_enumerate_add_match_subsystem( enumerate, POWER_SUPPLY_SUBSYSTEM_STR );
	udev_enumerate_scan_devices( enumerate );
	devices = udev_enumerate_get_list_entry( enumerate );

	/* Get the number of devices */
	udev_list_entry_foreach( dev_list_entry, devices ) {
		++ i;
	}

	/* Allocate the new devices */
	ses->ndevices = i;
	ses->devices = calloc( i, sizeof( struct PowerSupply ) );

	i = 0;
	/* Convert the udev devies to normal power supply
	 * devices */
	udev_list_entry_foreach( dev_list_entry, devices ) {
		/* Get the device path and use than */
		ses->devices[i].dev_path = strdup( udev_list_entry_get_name( dev_list_entry ) );

		/* create the new device */
		ses->devices[i].udev = udev_device_new_from_syspath( ses->udev,
			ses->devices[i].dev_path );

		/* Get the type of device . E.g. either a battery or
		 * an adapter */
		tmp = udev_device_get_sysattr_value( ses->devices[i].udev, "type" );

		if ( ! strcmp( tmp, BATTERY_TYPE_STR ) ) {
			/* The type is a battery */
			ses->devices[i].type = PowerSupplyType_Battery;

			/* Set some of the battery attributes */
			Create_Battery( & ses->devices[i] );

		} else if( ! strcmp( tmp, ADAPTER_TYPE_STR ) ) {
			/* The device is an adapter */
			ses->devices[i].type = PowerSupplyType_Adapter;

			/* Set the adapter-specific fields */
			/* TODO, break this into its own function */
			tmp = udev_device_get_sysattr_value( ses->devices[i].udev, "online" );
			ses->devices[i].adapter.online = strtol( tmp, NULL, 10 );
		} else {
			/* I'm not sure what the power supply is */
			ses->devices[i].type = PowerSupplyType_Unknown;
		}
		++ i;
	}
	
	/* Some basic cleanup */
	// udev_device_unref( dev );
	udev_enumerate_unref( enumerate );
	pthread_mutex_unlock( & ses->m_mutex ) ;
}

struct PowerSupply* PowerSupplySession_GetDeviceBySysPath
	( struct PowerSupply_Session* ses, const char* path ) {

	pthread_mutex_lock( & ses->m_mutex );
	/* Not the most efficient way to do this, but what are the
	 * chances of having throusands of different batteries? */
	size_t i = 0;
	for ( ; i < ses->ndevices ; ++ i ) {
		if( ! strcmp( ses->devices[i].dev_path, path ) ) {
			pthread_mutex_unlock( & ses->m_mutex );
			return &ses->devices[i] ;
		}
	}

	pthread_mutex_unlock( & ses->m_mutex );
	return NULL;
}

/* Main function for the monitoring thread. This
 * function will listen for events and call the
 * handling routines for each device */
void* monitor_main( void* args ) {
	int fd;
	int ret;
	struct udev_device* dev;
	fd_set fds;
	const char* syspath;
	struct PowerSupply* device;
	struct PowerSupply_Session* ses = (struct PowerSupply_Session*)args;

	/* I don't think I need an event struct, but just in case */
	struct PowerSupplyEvent evt;
	evt.type = PowerSupplyEventType_Plugged;

	/* Create the monitor */
	ses->monitor = udev_monitor_new_from_netlink( ses->udev, "udev" );

	/* Match events from the power_supply subsystem */
	udev_monitor_filter_add_match_subsystem_devtype
		( ses->monitor, POWER_SUPPLY_SUBSYSTEM_STR, NULL );

	/* enable the monitor */
	udev_monitor_enable_receiving( ses->monitor );
	fd = udev_monitor_get_fd( ses->monitor );

	while( 1 ) {
		/* Infinite loop... Wait for an event and
		 * when it arrives, iterate through the devices
		 * and call its callback */

		FD_ZERO( &fds );
		FD_SET( fd, &fds );

		ret = select( fd + 1, &fds, NULL, NULL, NULL );
	
		if( ret > 0 && FD_ISSET(fd, &fds)) {
//			DEBUG( "There is supposed to be an event now\n" );
			dev = udev_monitor_receive_device( ses->monitor ) ;
			if( dev ) {
				/* Get the device that raised the interrupt */
				syspath = udev_device_get_syspath( dev );

				/* Get the pseudo device mapped to the device */
				device = PowerSupplySession_GetDeviceBySysPath( ses, syspath );

				if( device && device->EventDetected ) {
					/* If the device exists and the Handling routine
					 * also exists, then call it */
					device->EventDetected( device, &evt );
				}
			}
		}
	}

	/* Return nothing */
	return NULL;
}

/* Starts the monitoring for the session */
void PowerSupplySession_StartMonitoring( struct PowerSupply_Session* ses ) {

	/* Create the new thread */
	pthread_create( &ses->monitoring_thread, NULL, monitor_main, ses );
}

void EventFlip( struct PowerSupply* ths,
	struct PowerSupplyEvent* event ) {
	printf( "From: %s\n", ths->dev_path );
	printf( "   OMG! There was an event!\n" );
}

inline double Battery_FractionFull( struct Battery* bat ) {
	return bat->energy_now / (double) bat->energy_full;
}

/* Testing main function */
// int main( int argc, char** argv ) {
// 	struct PowerSupply_Session* ses = PowerSupplySession_New();
// 	size_t i = 0;
// 
// 	if( ses ) {
// 		PowerSupplySession_GetDeviceList( ses );
// 		for( ; i < ses->ndevices; ++ i ) {
// 			ses->devices[i].EventDetected = EventFlip;
// 
// 			switch( ses->devices[i].type ) {
// 				case PowerSupplyType_Battery:
// 					printf("Battery Detected\n");
// 					printf("%.02f% remaining\n",
// 						Battery_FractionFull( &ses->devices[i].battery ) * 100 );
// 					break;
// 				case PowerSupplyType_Adapter:
// 					printf("Adapter Detected\n");
// 					printf("State:%s\n",
// 						ses->devices[i].adapter.online ? "Charging" : "Unplugged" );
// 					break;
// 				case PowerSupplyType_Unknown:
// 					printf("Unknown Device Detected\n");
// 					break;
// 			}
// 		}
// 	} else {
// 		perror( "Unable to open power supply session" );
// 	}
// 
// 	PowerSupplySession_StartMonitoring( ses );
// 	pthread_join( ses->monitoring_thread, NULL );
// 	return 0;
// }
