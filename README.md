# straight-httpd

Simple httpd demo for embedded systems based on lwip and mbedtls

# Objective

In a Windows environment, create a `Virtual Device (VD)` using `pcap`, `lwip` and `mbedtls`, and implement a simple but typical web server.

  ![stack](/stack.png)

This project creates a `Virtual Device` on `Computer B`. Since the browser on `Computer B` cannot access this local `Virtual Device`, another `Computer A` should be used to access this device remotely. 
* `Computer B` MUST be a Windows device with `VS2010` or later installed. Use VS2010 to open the project `straight-httpd/straight-httpd.sln` and run it in debug.
* `Computer A` could be a `Windows`, `Linux`, or a `mobile` device. For Windows, open `File Explorer` and search for `network neighbors`. There should be a device named `Virtual Device`. Right click on it and select `Properties`, and you will see the device information, including its IP address and URL. Use default user `admin` and password `password` to sign into the device home page.

  ![discover](/discover.png)  

# Features

* Support `SSDP` protocol: neighbours can detect this virtual device in the network.
* Support `HTTPS`: all HTTP requests will be redirected to HTTPS (except SSDP XML). Both `GET` and `POST` are supported.
* Support `chunked` response: Because the device memory is limited, generally it is not possible to wait for all the data to be ready before replying. When the device begins to return data, it does not know the total length. `Transfer-Encoding: chunked` allows you to split the data into chunks of various lengths, so that every chunk is within the device’s memory limit.
* Support `range` request: `Range: bytes=0-` allows you to specify a single range. This feature is suitable for paged data, multi-threaded downloading or video fast forwarding.
* Support request header `If-Modified-Since` for browsers to cache static files.
* Support simple SSI (“Server Side Includes”) with simple syntax: `<!--#TAG_NAME-->`. Note that the syntax does not allow spaces between the "<" and ">". Every tag is a placeholder, which corresponds to a variable. The variable can be HTML content or the initial value of a javascript variable. When the device replies to a request, it will first convert the variable into a string. This string then replaces the corresponding tag. Based on the tag content, the js code can then complete HTML/DOM operations, such as select, checkbox, and radio control etc.
* Demo pages include:
  * **Sign in/out**: verify user's username/password, if successful, redirect to "**Home**", otherwise stay in the login page, the session has 3 minutes timeout after login.
  * **Home** page:  show some device information and the current status of the device.
  * **Upload** page: multiple files can be selected and queued to be uploaded to a specific device directory. The concurrency is 1, and every upload process can be terminated separately.
  * **Files** page: you can browse all the files in the specified device directory, enter subdirectories or return to the parent directory.
  * **Form** page: demonstrates parameter modification and saving.

# Future work

* Spport MQTT protocol.
* Port httpd to a real embedded system, such as Cortex-M4 LPC40xx.

# Prerequisites

* `Computer A`: only need a browser installed. If using the latest npcap driver (v0.9995), the browser on `Computer B` can access the `Virtual Device` right now, so `Computer A` may not be needed.
* `Computer B`: need to install [`wpcap driver`](https://nmap.org/npcap/dist/npcap-0.9995.exe), or simply install [`Wireshark`](https://www.wireshark.org/) that includes wpcap driver.

# Dependencies

* folder `pcap`: `wpcap` C library copied from the original location: [pcap](https://nmap.org/npcap/dist/npcap-sdk-1.04.zip)
* folder `lwip`: copied from the original location: **git://git.savannah.nongnu.org/lwip.git**
* folder `mbedtls`: copied from the original location: [mbedtls](https://github.com/ARMmbed/mbedtls.git). The file config.h has been modified.
* `QR-Code-generator` by nayuki, copied from the original location: [source](https://github.com/nayuki/QR-Code-generator/tree/a6ef65d237628a03dee3ae1df592df9a3359204d/javascript)

# Project folders

* folder `lwip-port`: lwip porting files on Windows platform
  * Receive ethernet frame from `wpcap` and send it to `lwip stack`.
  * Send ethernet frame from `lwip` stack via `wpcap`.
  * Also provide features: `Mutex`, `Semaphore`, `Mailbox`, `System Tick`, and `File API`.

* folder `straight-httpd`: Microsoft Visual Studio 2010 project: `straight-httpd.sln`, and the project `straight-buildfs` can package all web pages and convert them into source code.

# Self-signed certificate for HTTPS

* If you want to create your customized certificate, run `makecert.bat`. Both the certificate and the private key will be saved in the file `server.pfx`.
*	Use [**OpenSSL**](https://slproweb.com/products/Win32OpenSSL.html) or other tools to convert PFX file to PEM format.
```
   openssl pkcs12 -in server.pfx -out server.pem
```
*	Copy the following content in the pem file to `http_api.c`
```
const char *privkey_pass = "straight";
const char *privkey = "-----BEGIN ENCRYPTED PRIVATE KEY-----\n"\
"MIIJjjBABgkqhkiG9w0BBQ0wMzAbBgkqhkiG9w0BBQwwDgQIh7VHf699+v0CAggA\n"\
    。。。。
"-----END ENCRYPTED PRIVATE KEY-----\n";

const char *cert = "-----BEGIN CERTIFICATE-----\n"\
"MIIFEDCCAvigAwIBAgIQ+UjKCIcDNLRKcHf+9tlCFjANBgkqhkiG9w0BAQ0FADAR\n"\
    。。。。
"-----END CERTIFICATE-----\n";
```
# Task / Thread

* LWIP task can be thought as a black-box that is asynchronously driven by the network events. 
* Immediate response is very important for high throughput.
* This task need access various data in local database or variables which should be protected for concurrency.

  ![tasks](/tasks.png)

# Modifications

* `/src/include/lwip/priv/tcpip_priv.h`
```
ETHNET_INPUT //2020-03-29 added by straight coder
```
* `/src/include/lwip/arch.h`
```
define LWIP_NO_STDINT_H 1	//2020-03-29 set to 1 by straight coder
```
* `/src/core/tcp.c`
```
void tcp_kill_all(void)  //2020-03-29 new function by straight coder
```
* `/src/core/ipv4/dhcp.c`
```
OnDhcpFinished();   //2020-03-29 notify when IP is ready by straight coder
```
* `/src/api/tcpip.c`
```
sys_mbox_t tcpip_mbox; //2020-03-29 removed static by straight coder	
extern struct netif main_netif; //2020-03-29 added by straight coder
extern err_t ethernetif_input(struct netif *netif); //2020-03-29 added by straight coder
void tcpip_thread_handle_msg(struct tcpip_msg *msg); //2020-03-29 removed static by straight coder
	
case ETHNET_INPUT: //2020-03-29 added by straight coder
   ethernetif_input(&main_netif);
   break;
```
* for pcap
```
#define ETH_PAD_SIZE  0 
struct member alignment 1 byte(/Zp1)
```

# Structure for CGI mapping

```
    struct CGI_Mapping
    {
	char path[MAX_CGI_PATH];	//string, request full path
	unsigned long optionsAllowed;	//defined by CGI_OPT_xxxxxx

	void (*OnCancel)(REQUEST_CONTEXT* context);

	//request callbacks
	int  (*OnHeaderReceived)(REQUEST_CONTEXT* context, char* header_line); //return 1 if eaten
	void (*OnHeadersReceived)(REQUEST_CONTEXT* context);
	int  (*OnContentReceived)(REQUEST_CONTEXT* context, char* buffer, int size);
	void (*OnRequestReceived)(REQUEST_CONTEXT* context);
	
	//response callbacks
	void (*SetResponseHeaders)(REQUEST_CONTEXT* context, char* HttpCode);
	int  (*ReadContent)(REQUEST_CONTEXT* context, char* buffer, int maxSize);
	void (*OnAllSent)(REQUEST_CONTEXT* context);
	
	void (*OnFinished)(REQUEST_CONTEXT* context);
	
	struct CGI_Mapping* next;
    };
```
 * Functions for CGI processing
``` 
    //setup mapping when initializing httpd context
    void CGI_SetupMapping(void); 
   
    //cancel notification to app layer because of any HTTP fatal errors
    //  including timeout, format errors, sending failures, and stack keneral errors
    void CGI_Cancel(REQUEST_CONTEXT* context);

    //called by FreeHttpContext()
    void CGI_Finish(REQUEST_CONTEXT* context);

    //called when the first line of the HTTP request is completely received
    void CGI_SetCgiHandler(REQUEST_CONTEXT* context);

    //if any HTTP request header/line is skipped by http-core.c, the following function will be called, return 1 if accepted
    int CGI_HeaderReceived(REQUEST_CONTEXT* context, char* header_line);

    //called when all HTTP request headers/lines are received
    void CGI_HeadersReceived(REQUEST_CONTEXT* context);

    //called when any bytes of the HTTP request body are received (may be partial), and return the accepted count.
    int CGI_ContentReceived(REQUEST_CONTEXT* context, char* buffer, int size);

    //called when the HTTP request body is completely received
    void CGI_RequestReceived(REQUEST_CONTEXT* context);

    //set response headers: content-type, content-length, connection, and http code and status info
    void CGI_SetResponseHeaders(REQUEST_CONTEXT* context, char* HttpCodeInfo);

    //load response body chunk by chunk
    //  there are two-level progresses initialized as 0: 
    // 	  context->ctxResponse._dwOperStage = 0; //major progress, set _dwOperStage=STAGE_END if it is the last data chunk
    //	  context->ctxResponse._dwOperIndex = 0; //sub-progress for each stage, for app layer internal use
    //    caller: called from HTTP_PROC_CALLER_RECV (event) / HTTP_PROC_CALLER_POLL (timer) / HTTP_PROC_CALLER_SENT (event)
    //  return 1 if data is ready to send
    //	  data MUST be put in context->ctxResponse._sendBuffer, 
    //         and the buffer size set to context->ctxResponse._bytesLeft before return
    int CGI_LoadContentToSend(REQUEST_CONTEXT* context, int caller);
```

# GET handling

| Device HTTP Events | CGI Adapter - Actions | Description |
| ------------ |:-------------:| -------------:|
| GET /app/index.shtml HTTP/1.1 |	CGI_SetCgiHandler(context)	| First request line |
| Connection: keep-alive<br>Cookie: SID=0123ABCD<br>X-Auth-Token: SID=0123ABCD<br>Range: bytes=0-<br>If-Modified-Since:	| processed in http_core.c | Request headers  |
| other headers	| CGI_HeaderReceived(context,line)		| Request headers |
| all headers received	| Check session: GetSession(char* token)<br>CGI_HeadersReceived(context)	| End of the request headers	|
| request completely received		| CGI_RequestReceived(context)		| Ready to response  |
| send response headers		| CGI_SetResponseHeaders(context,codeInfo)		| Response headers	| 
| response with content		| CGI_LoadContentToSend(context, caller)		| Response body	| 
| Response completely sent out		| OnAllSent(context)		| Response finished	|  
| When connection disconnected or -500		| CGI_Finish(context)		| For additional cleanup	|  
| lwip error, request error, timeout, <=-400		| CGI_Cancel(context)		| For additional cleanup	|  

# POST handling

| Device HTTP Events | CGI Adapter - Actions | Description |
| ------------ |:-------------:| -------------:|
| POST /auth/login.html HTTP/1.1	| CGI_SetCgiHandler(context)	| First request line	| 
| Content-Type: application/x-form-urlencoded<br>Content-Length: xxx<br>Cookie: SID=0123ABCD<br>X-Auth-Token: SID=0123ABCD<br>Connection: keep-alive	| processed in http_core.c	| Request headers	| 
| other headers	| CGI_HeaderReceived(context,line)		| Request headers	| 
| all headers received	| Check session: GetSession(char* token)<br>CGI_HeadersReceived(context)	| End of the request headers	| 
| request body	| CGI_ContentReceived(context, buffer, size)	| Request body	| 
| post body received	| CGI_RequestReceived(context)	| Ready to response	| 	
| send response headers	| CGI_SetResponseHeaders(context,codeInfo)	| Response headers	| 
| response with content	| CGI_LoadContentToSend(context, caller)	| Response body	| 
| Response completely sent out	| OnAllSent(context)		| Response finished	| 
| When connection disconnected or -500	| CGI_Finish(context)		| For additional cleanup	| 
| lwip error, request error, timeout, <=-400	| CGI_Cancel(context)		| For additional cleanup	| 

# Example for SSDP response

* cgi_ssdp.c processes SSDP requests. 
* All information tags can be modified using API functions. All tags are defined and parsed by `cgi_ssi.c`.
```
    e.g. GetDeviceName(), GetVendor(), GetModel(), GetDeviceUUID(). 
```
  ![discover-prop](/discover-prop.png)

# Example for sign-in UI

* `/auth/login.html` is the default page before authentication. The user must provide the username and password. All web pages are physically located in the folder defined by `LOCAL_WEBROOT`.
```
#define LOCAL_WEBROOT	"D:/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/"
```
* `cgi_auth.c` processes the user’s sign-in/sign-out. This module responds to all requests with the prefix `/auth/*`.
* `/auth/session_check.cgi` checks if the current session has timed out.
* `/auth/logout.cgi` signs out the user and frees the session context.
* Default user: `admin`, password: `password`
```
    int CheckUser(char* u, char* p);
```
  ![signin](/signin.png)

# Example for status and information

* `/app/index.shtml` is the home page after authentication.
* `cgi_web.c` responds to all requests with prefix `/app/*` after authentication.
* `cgi_ssi.c` includes all infomation getters.
```
    char* GetVendor(void);
    char* GetVendorURL(void);
    char* GetModel(void);
    char* GetModelURL(void);
    char* GetDeviceName(void);
    char* GetDeviceSN(void);
    char* GetDeviceUUID(void);
    char* GetDeviceVersion(void);
    int FillMAC(void* context, char* buffer, int maxSize);
    
    static SSI_Tag g_Getters[] = {
	{ "DEV_VENDOR",		TAG_GETTER, GetVendor },
	{ "DEV_VENDOR_URL",	TAG_GETTER, GetVendorURL },
	{ "DEV_MODEL",		TAG_GETTER, GetModel },
	{ "DEV_MODEL_URL",	TAG_GETTER, GetModelURL },
	{ "DEV_MAC",		TAG_PROVIDER, FillMAC },
	{ "DEV_NAME",		TAG_GETTER, GetDeviceName },
	{ "DEV_SN",		TAG_GETTER, GetDeviceSN },
	{ "DEV_UUID",		TAG_GETTER, GetDeviceUUID },
	{ "DEV_VERSION",	TAG_GETTER, GetDeviceVersion },
	{ NULL, NULL, NULL }
    };
```
  ![info](/info.png)

# Example for uploading

* `/app/upload.shtml` and `/app/plugin/fileTransfer/fileTransfer.js` provide a demo for uploading files. The destination folder is defined by `UPLOAD_TO_FOLDER`.
```
#define UPLOAD_TO_FOLDER "D:/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/app/cache/"
```
* `cgi_upload.c` responds to the request URL `/api/upload.cgi`.
```
    long Upload_Start(void* context, char* szFileName, long nFileSize);
    long Upload_GetFreeSize(unsigned long askForSize);
    long Upload_Received(REQUEST_CONTEXT* context, unsigned char* pData, unsigned long dwLen);
    void Upload_AllReceived(REQUEST_CONTEXT* context);
```
  ![upload](/upload.png)

# Example for file explorer

* This is a good example for `RESTful API` using `chunked` header.
* `/app/files.shtml` and `/app/plugin/fileList/fileList.js` provide a demo for file browsing. The directory is defined by `FOLDER_TO_LIST`.
```
#define FOLDER_TO_LIST	"D:/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/app/cache/"
```
* `cgi_files.c` responds to the request with URL `/api/files.cgi`. 
```
    context->_options |= CGI_OPT_CHUNK_ENABLED;
```
* Makes preparations before response.
```
   static void Files_OnRequestReceived(REQUEST_CONTEXT* context);
```
* Generates response header with `chunked` header.
```
   static void Files_SetResponseHeader(REQUEST_CONTEXT* context, char* HttpCodeInfo);
```
* Generates one small `chunk` on each call. 
   * `static int Files_ReadOneFileInfo(REQUEST_CONTEXT* context, char* buffer, int maxSize);`
   * This function is a callback from lwip stack when it is ready to send the next `chunk`.
   * There are two variables in the context that could be used for the progress control.
     * `context->ctxResponse._dwOperStage`: It is the first level progress (0-based), and `STAGE_END` stands for the last `chunk`.
     * `context->ctxResponse._dwOperIndex`: Optional, it is the second level progress (0-based), and the max value is `context->ctxResponse._dwTotal`.
```
request:
  http://192.168.5.58/api/files.cgi?path=/
response with JSON including 9 chunks:
```
```
  {
    "result":1, 
    "data":[
```
```
      {"folder":1,"name":"..","size":0,"date":1593903275},
```
```
      {"folder":0,"name":"00108.MTS","size":147062784,"date":1596318548},
```
```
      {"folder":0,"name":"Building a 3d Printer From Cd Drives -- Only $45.mp4","size":90868943,"date":1595116269},
```
```
      {"folder":1,"name":"cfg","size":0,"date":1593903275},
```
```
      {"folder":1,"name":"include","size":0,"date":1593903276},
```
```
      {"folder":1,"name":"lib","size":0,"date":1593903276},
```
```
      {"folder":0,"name":"videoplayback.mp4","size":25542302,"date":1593903614}
```
```
    ]
  }
```
  ![files](/files.png)

# Example for configuration form

* `/app/form.shtml` is a demo for modifying parameters. All parameters are processed by `cgi_ssi.c`.
* `cgi_form.c` provides general processing for all forms. All parameters and types are defined in `cgi_ssi.c`.
* `cgi_ssi.c` includes all infomation getters and setters.
```
    static SSI_Tag g_Getters[] = {
	{ "DEV_DHCP",		TAG_PROVIDER, FillDhcp },
	{ "DEV_IP",		TAG_PROVIDER, FillIP },
	{ "DEV_GATEWAY",	TAG_PROVIDER, FillGateway },
	{ "DEV_SUBNET",		TAG_PROVIDER, FillSubnet },
	{ "VAR_SESSION_TIMEOUT",TAG_PROVIDER, FillSessionTimeout },
	{ "VAR_LOCATION",	TAG_GETTER, GetLocation },
	{ "VAR_COLOR",		TAG_GETTER, GetColor },
	{ "VAR_DATE",		TAG_GETTER, GetDate },
	{ "VAR_FONT",		TAG_GETTER, GetFont },
	{ "VAR_LOG",		TAG_PROVIDER, FillLog },
	{ NULL, NULL, NULL }
    };

    static SSI_Tag g_Setters[] = {
	{ "DEV_DHCP",		TAG_SETTER, SetDhcpEnabled },
	{ "DEV_IP",		TAG_SETTER, SetMyIP },
	{ "DEV_GATEWAY",	TAG_SETTER, SetGateway },
	{ "DEV_SUBNET",		TAG_SETTER, SetSubnet },
	{ "VAR_SESSION_TIMEOUT",TAG_SETTER, SetSessionTimeout },
	{ "VAR_LOCATION",	TAG_SETTER, SetLocation },
	{ "VAR_COLOR",		TAG_SETTER, SetColor },
	{ "VAR_DATE",		TAG_SETTER, SetDate },
	{ "VAR_FONT",		TAG_SETTER, SetFont },
	{ "VAR_LOG",		TAG_SETTER, SetLog },
	{ NULL, NULL, NULL }
    };

    void LoadConfig4Edit();
    void ApplyConfig();
```
  ![form](/form.png)
