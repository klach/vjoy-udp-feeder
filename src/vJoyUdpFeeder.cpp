// vJoyUdpFeeder.cpp : Simple feeder application
//
// Supports both types of POV Hats

#include "stdafx.h"
#include "public.h"
#include "vjoyinterface.h"
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#pragma comment( lib, "VJOYINTERFACE" )
#define  _CRT_SECURE_NO_WARNINGS

int
__cdecl
_tmain(__in int argc, __in PZPWSTR argv)
{
	_tprintf("Usage: vJoyUdpFeeder [device number] [port]\n\n");

	USHORT X, Y, Z, ZR, XR;							// Position of several axes
	JOYSTICK_POSITION	iReport;					// The structure that holds the full position data
	BYTE id=1;										// ID of the target vJoy device (Default is 1)
	UINT iInterface=1;								// Default target vJoy device
	BOOL ContinuousPOV=FALSE;						// Continuous POV hat (or 4-direction POV Hat)
	int count=0;
	int argPort=0;


	// Get the ID of the target vJoy device
	if (argc>1 && wcslen(argv[1]))
		sscanf_s((char *)(argv[1]), "%d", &iInterface);

	if (argc>2 && wcslen(argv[2]))
		sscanf_s((char *)(argv[2]), "%d", &argPort);


	// Get the driver attributes (Vendor ID, Product ID, Version Number)
	if (!vJoyEnabled())
	{
		_tprintf("vJoy driver not enabled: Failed getting vJoy attributes.\n");
		return -2;
	}
	else
	{
		_tprintf("Vendor: %S\nProduct: %S\nVersion number: %S\n", TEXT(GetvJoyManufacturerString()),  TEXT(GetvJoyProductString()), TEXT(GetvJoySerialNumberString()));
	};

	// Get the state of the requested device
	VjdStat status = GetVJDStatus(iInterface);
	switch (status)
	{
	case VJD_STAT_OWN:
		_tprintf("vJoy device %d is already owned by this feeder\n", iInterface);
		break;
	case VJD_STAT_FREE:
		_tprintf("vJoy device %d is free\n", iInterface);
		break;
	case VJD_STAT_BUSY:
		_tprintf("vJoy device %d is already owned by another feeder\nCannot continue\n", iInterface);
		return -3;
	case VJD_STAT_MISS:
		_tprintf("vJoy device %d is not installed or disabled\nCannot continue\n", iInterface);
		return -4;
	default:
		_tprintf("vJoy device %d general error\nCannot continue\n", iInterface);
		return -1;
	};

	// Check which axes are supported
	BOOL AxisX  = GetVJDAxisExist(iInterface, HID_USAGE_X);
	BOOL AxisY  = GetVJDAxisExist(iInterface, HID_USAGE_Y);
	BOOL AxisZ  = GetVJDAxisExist(iInterface, HID_USAGE_Z);
	BOOL AxisRX = GetVJDAxisExist(iInterface, HID_USAGE_RX);
	BOOL AxisRZ = GetVJDAxisExist(iInterface, HID_USAGE_RZ);
	// Get the number of buttons and POV Hat switchessupported by this vJoy device
	int nButtons  = GetVJDButtonNumber(iInterface);
	int ContPovNumber = GetVJDContPovNumber(iInterface);
	int DiscPovNumber = GetVJDDiscPovNumber(iInterface);

	// Print results
	_tprintf("\nvJoy device %d capabilities:\n", iInterface);
	_tprintf("Number of buttons\t\t%d\n", nButtons);
	_tprintf("Number of Continuous POVs\t%d\n", ContPovNumber);
	_tprintf("Number of Discrete POVs\t\t%d\n", DiscPovNumber);
	_tprintf("Axis X\t\t%s\n", AxisX?"Yes":"No");
	_tprintf("Axis Y\t\t%s\n", AxisX?"Yes":"No");
	_tprintf("Axis Z\t\t%s\n", AxisX?"Yes":"No");
	_tprintf("Axis Rx\t\t%s\n", AxisRX?"Yes":"No");
	_tprintf("Axis Rz\t\t%s\n", AxisRZ?"Yes":"No");



	// Acquire the target
	if ((status == VJD_STAT_OWN) || ((status == VJD_STAT_FREE) && (!AcquireVJD(iInterface))))
	{
		_tprintf("Failed to acquire vJoy device number %d.\n", iInterface);
		return -1;
	}
	else
	{
		_tprintf("Acquired: vJoy device number %d.\n", iInterface);
	}



	_tprintf("\nPress enter to start feeding...\n");
	getchar();

	X = 20;
	Y = 30;
	Z = 40;
	XR = 60;
	ZR = 80;

	WSAData data;
	if (WSAStartup( MAKEWORD( 2, 2 ), &data ) != 0) {
		_tprintf("Could not open Windows connection.\n");
		return -1;
	}

	struct sockaddr_in server;
	struct sockaddr_in client;
	char buf[1024];
	unsigned short server_port = (argPort < 1024 || argPort > 65535) ? 1608 : argPort;
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		_tprintf("Could not create socket.\n");
		WSACleanup();
		return -1;
	}
	memset(&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(server_port);
	if (bind(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0) {
		_tprintf("Could not bind socket.\n");
		closesocket(sock);
		WSACleanup();
		return -1;
	}
	_tprintf("Listening on UDP port %d.\n", server_port);
	int client_length = sizeof(struct sockaddr_in);

	long value = 0;
	BOOL res = FALSE;

	// Start feeding in an endless loop
	while (1)
	{
		int bytes_received = recvfrom(sock, buf, 1024, 0, (struct sockaddr *)&client, &client_length);
		if (bytes_received < 0) {
			_tprintf("Could not receive datagram.\n");
			continue;
		}

		/*** Create the data packet that holds the entire position info ***/
		id = (BYTE)iInterface;
		iReport.bDevice = id;

		iReport.wAxisX=X;
		iReport.wAxisY=Y;
		iReport.wAxisZ=Z;
		iReport.wAxisZRot=ZR;
		iReport.wAxisXRot=XR;

		// Set buttons one by one
		iReport.lButtons = 1<<count/20;

		if (ContPovNumber)
		{
			// Make Continuous POV Hat spin
			iReport.bHats		= (DWORD)(count*70);
			iReport.bHatsEx1	= (DWORD)(count*70)+3000;
			iReport.bHatsEx2	= (DWORD)(count*70)+5000;
			iReport.bHatsEx3	= 15000 - (DWORD)(count*70);
			if ((count*70) > 36000)
			{
				iReport.bHats = -1; // Neutral state
				iReport.bHatsEx1 = -1; // Neutral state
				iReport.bHatsEx2 = -1; // Neutral state
				iReport.bHatsEx3 = -1; // Neutral state
			};
		}
		else
		{
			// Make 5-position POV Hat spin
			unsigned char pov[4];
			pov[0] = ((count/20) + 0)%4;
			pov[1] = ((count/20) + 1)%4;
			pov[2] = ((count/20) + 2)%4;
			pov[3] = ((count/20) + 3)%4;

			iReport.bHats		= (pov[3]<<12) | (pov[2]<<8) | (pov[1]<<4) | pov[0];
			if ((count) > 550)
				iReport.bHats = -1; // Neutral state
		};

		/*** Feed the driver with the position packet - is fails then wait for input then try to re-acquire device ***/
		if (!UpdateVJD(iInterface, (PVOID)&iReport))
		{
			_tprintf("Feeding vJoy device number %d failed - try to enable device then press enter\n", iInterface);
			getchar();
			AcquireVJD(iInterface);
			ContinuousPOV = (BOOL)GetVJDContPovNumber(iInterface);
		}

				Sleep(20);
		count++;
		if (count > 640) count=0;

		X+=150;
		Y+=250;
		Z+=350;
		ZR-=200;

	};

	closesocket(sock);
	WSACleanup();

	_tprintf("OK\n");

	return 0;
}
