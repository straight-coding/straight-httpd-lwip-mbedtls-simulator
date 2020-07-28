# straight-httpd

Simple httpd demo for embedded systems based on lwip and mbedtls

# Objective

In a Windows environment, create a `Virtual Device (VD)` using `pcap`, `lwip` and `mbedtls`, and implement a simple but typical web server.

  ![stack](/stack.png)

This project creates a `Virtual Device` on `Computer B`. Since the browser on `Computer B` cannot access this local `Virtual Device`, another `Computer A` should be used to access this device remotely. 
* `Computer B` MUST be a Windows device with `VS2017` or later installed. Use VS2017 to open the project `straight-httpd/straight-httpd.sln` and run it in debug.
* `Computer A` could be a `Windows`, `Linux`, or a `mobile` device. For Windows, open `File Explorer` and search for `network neighbors`. There should be a device named `Virtual Device`. Right click on it and select `Properties`, and you will see the device information, including its IP address and URL. Use default user `admin` and password `password` to sign into the device home page.

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

* `Computer A`: only need a browser installed.
* `Computer B`: need to install [`wpcap driver`](https://www.winpcap.org/), or simply install [`Wireshark`](https://www.wireshark.org/) that includes wpcap driver.

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

* folder `straight-httpd`: Microsoft Visual Studio 2017 project: `straight-httpd.sln`, and the project `straight-buildfs` can package all web pages and convert them into source code.

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
# Example for sign-in UI

* `/auth/login.html` is the default page before authentication. The user must provide the username and password. All web pages are physically located in the folder defined by `LOCAL_WEBROOT`.
```
#define LOCAL_WEBROOT	"D:/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/"
```
* `cgi_auth.c` processes the user’s sign-in/sign-out. This module responds to all requests with the prefix `/auth/*`.
* `/auth/session_check.cgi` checks if the current session has timed out.
* `/auth/logout.cgi` signs out the user and frees the session context.
* Default user: `admin`, password: `password`

  ![signin](/signin.png)

# Example for status and information

* `/app/index.shtml` is the home page after authentication.
* `cgi_web.c` responds to all requests with prefix `/app/*` after authentication.

  ![info](/info.png)

# Example for uploading

* `/app/upload.shtml` and `/app/plugin/fileTransfer/fileTransfer.js` provide a demo for uploading files. The destination folder is defined by `UPLOAD_TO_FOLDER`.
```
#define UPLOAD_TO_FOLDER "D:/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/app/cache/"
```
* `cgi_upload.c` responds to the request URL `/api/upload.cgi`.

  ![upload](/upload.png)

# Example for file explorer

* `/app/files.shtml` and `/app/plugin/fileList/fileList.js` provide a demo for file browsing. The directory is defined by `FOLDER_TO_LIST`.
```
#define FOLDER_TO_LIST	"D:/straight/straight-httpd/straight-httpd/straight-httpd/httpd/cncweb/app/cache/"
```
* `cgi_files.c` responds to the request with URL `/api/files.cgi`

  ![files](/files.png)

# Example for configuration form

* `/app/form.shtml` is a demo for modifying parameters. All parameters are processed by `cgi_ssi.c`.
* `cgi_form.c` provides general processing for all forms. All parameters and types are defined in `cgi_ssi.c`.

  ![form](/form.png)
