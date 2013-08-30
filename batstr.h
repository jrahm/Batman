#ifndef BATSTR_H_
#define BATSTR_H_

#include <stdint.h>
#include <libudev.h>

#include <pthread.h>

/* The different kinds of power supplies
 * we can use */
enum PowerSupplyType {
	  PowerSupplyType_Adapter
	, PowerSupplyType_Battery
	, PowerSupplyType_Unknown
};

/* Types of PowerSupply events.
 * Deprecated */
enum PowerSupplyEventType {
	  PowerSupplyEventType_Unplugged
	, PowerSupplyEventType_Plugged
};

/* Event type */
struct PowerSupplyEvent {
	enum PowerSupplyEventType type;
};

struct Adapter {
	/* 1 if the adapter is plugged in */
	int online;
};

struct Battery {
	int alarm;

	/* Name of the manufacturer */
	char* manufacturer;

	/* Amount of energy this Battery can hold
	 * when full */
	uint32_t energy_full;

	/* Amount of energy this Battery is holding
	 * how */
	uint32_t energy_now;

	/* Current Power */
	uint32_t power_now;

	/* Technology */
	char* technology;

	/* Status */
	char* status;

	/* Voltage */
	uint32_t voltage;
};

/* A super struct of the Battery and Adapter
 * structs which holds information about 
 * the udev device */
struct PowerSupply {
	/* Type of this power supply. Informs
	 * the application which in the union
	 * to use */
	enum PowerSupplyType type;

	/* The udev handles to this
	 * device */
	struct udev_device* udev;
	char* dev_path;
	
	/* Called when there's an event detected
	 * on the device */
	void (* EventDetected) ( struct PowerSupply* ths,
		struct PowerSupplyEvent* event );

	/* The mutex used to control access to the important
	 * values, which may be changing */
	pthread_t mutex;

	/* you define */
	void* udef;

	/* The device-specific parts of the
	 * udev */
	union {
		struct Adapter adapter;
		struct Battery battery;
	};
};

struct PowerSupply_Session {
	/* The connection to udev to use */
	struct udev *udev;

	/* The number of devices in the devices
	 * array */
	size_t ndevices;
	
	/* The array of devices */
	struct PowerSupply* devices;

	/* Thread that is monitoring values */
	pthread_t monitoring_thread;

	/* The monitor for the power_supply subsystem */
	struct udev_monitor* monitor;

	/* The mutex to control access */
	pthread_mutex_t m_mutex;
};

/*
 * Create a new session. This connects to the 
 * udev and sets up monitoring */
struct PowerSupply_Session* PowerSupplySession_New( );

/* Populates the session with new devices */
void PowerSupplySession_GetDeviceList( struct PowerSupply_Session* ses );

/* Starts the monitoring for the session */
void PowerSupplySession_StartMonitoring( struct PowerSupply_Session* ses );

void PowerSupply_Free( struct PowerSupply* power_supply );

/* Returns the device that matches the path in the session */
struct PowerSupply* PowerSupplySession_GetDeviceBySysPath
	( struct PowerSupply_Session* ses, const char* path );

/* Returns the percentage of energe the battery has
 * remaining */
inline double Battery_FractionFull( struct Battery* bat ) ;

#define PowerSupplySession_ForeachDevice( ses, dev ) \
	size_t garbleddeadbeefsomething5432 = 0; \
	for( ; garbleddeadbeefsomething5432 < (ses)->ndevices && \
				((dev) = &(ses)->devices[garbleddeadbeefsomething5432]) != NULL ; \
					++ garbleddeadbeefsomething5432 )

#endif
