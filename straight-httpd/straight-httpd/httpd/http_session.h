/*
  http_session.h
  Author: Straight Coder<simpleisrobust@gmail.com>
  Date: June 13, 2020
*/

#ifndef _HTTP_SESSION_H_
#define _HTTP_SESSION_H_

#define MAX_SESSIONS		5
#define MAX_COOKIE_SIZE		32		//max length of the cookie string
//#define TO_SESSION			3*60*1000

/////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _SESSION
{
	char _token[MAX_COOKIE_SIZE];	//for http_core.c

	unsigned long _nLoginIP;
	unsigned long long _tLastReceived;	//tick of the last received
	unsigned long long _tLastSent;		//tick of the last sent
}SESSION;	//size of SESSION: 44

void SetupSession(void);

SESSION* SessionCreate(char* outCookie, unsigned long remoteIP);
SESSION* GetSession(char* token);

void SessionKill(SESSION* session);
void SessionClearAll(void); //mutex used inside
int  SessionTypes(char* extension);

void SessionCheck(long long tick);
void SessionReceived(SESSION* session);
void SessionSent(SESSION* session);

#endif
