// Main.cpp : Defines the entry point for the console application.
//
//#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include "stdafx.h"
#include "arduino.h"

#pragma comment (lib, "Ws2_32.lib")

int _tmain(int argc, _TCHAR* argv[])
{
    return RunArduinoSketch();
}

#define DEFAULT_SERVER "169.254.224.76"
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int led = 13;  // This is the pin the LED is attached to.

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;

char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

int iResult;

float getOnboardTemp();
float getExternalSensorTemp();
int setupWinsockClient();
int setupWinsockServer();

void setup()
{
    // TODO: Add your code here
    
    pinMode(led, OUTPUT);       // Configure the pin for OUTPUT so you can turn on the LED.
	//setupWinsockClient();
	setupWinsockServer();
}

// the loop routine runs over and over again forever:
void loop()
{
    // TODO: Add your code here
	int iSendResult;

	digitalWrite(led, HIGH);   // turn the LED on by making the voltage HIGH
	Log(L"LED ON\n");
	//delay(1000);
	// read the temp reading from the onboard sensor and an external sensor
	float onboardTemp = getOnboardTemp();
	//setColor(ledDigitalOne, MAGENTA);
	Log(L"raw voltage reading: %lf\n", onboardTemp);
	//Sleep(1000);

	float externalTemp = getExternalSensorTemp();
	//setColor(ledDigitalOne, CYAN);
	Log(L"External Sensor Temperature: %lf Celcius\n", externalTemp);

	delay(4000);

	digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
	Log(L"LED OFF\n");
	delay(1000);

	//String deviceMessage = "device Id: " + 0;
	//String onBoardTempStr = "" + onboardTemp;
	boost::format fmter("%1%;%2%;%3%\n");
	fmter % 0 % onboardTemp % externalTemp;
	std::string ds = fmter.str();
	strcpy(recvbuf, ds.c_str());
	int len = ds.length();

	iSendResult = send(ClientSocket, recvbuf, len, 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		return;
	}

          // wait for a second
         // wait for a second
}

// per the online sample, the onboard temp sensor reads from pin -1 and is already covnerted to Celcius
float getOnboardTemp()
{
	int pin = 0;
	return analogRead(pin);
}

// the external sensor is assuming connection to analog pin 0. 
// Returns degrees in Celcius.
float getExternalSensorTemp()
{
	int pin = 0;
	float temp = analogRead(pin);

	//temp = temp * 0.004882814; //converting from a 0 to 1023 digital range
	//// to 0 to 5 volts (each 1 reading equals ~ 5 millivolts

	//return (temp - 0.5) * 100; //converting from 10 mv per degree with 500mV offset
	temp = temp * (5.0 / 4096.0);

	return 100 * temp - 50;
}

int setupWinsockServer()
{
	WSADATA wsaData;


	//SOCKET ListenSocket = INVALID_SOCKET;
	//SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;


	//char recvbuf[DEFAULT_BUFLEN];
	//int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	//setColor(ledDigitalOne, GREEN);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// No longer need server socket
	closesocket(ListenSocket);
}

int setupWinsockClient()
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	char *sendbuf = "this is a test";
	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Validate the parameters
	//if (argc != 2) {
	//	printf("usage: %s server-name\n", argv[0]);
	//	return 1;
	//}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		Log("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(DEFAULT_SERVER, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		Log("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			Log("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			Log("connect failed with error: %ld\n", WSAGetLastError());
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		Log("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	// Send an initial buffer
	iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iResult == SOCKET_ERROR) {
		Log("send failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}

	Log("Bytes Sent: %ld\n", iResult);

}
