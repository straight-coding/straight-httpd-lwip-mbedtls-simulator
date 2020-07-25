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

#	Prerequisites

* `Computer A`: only need a browser installed.
* `Computer B`: need to install wpcap driver, or simply install Wireshark that includes wpcap driver.

# Dependencies

* folder `pcap`: copied from the original location: [pcap](https://nmap.org/npcap/dist/npcap-sdk-1.04.zip)
* folder `lwip`: copied from the original location: **git://git.savannah.nongnu.org/lwip.git**
* folder `mbedtls`: copied from the original location: [mbedtls](https://github.com/ARMmbed/mbedtls.git)
* `QR-Code-generator` by nayuki, copied from the original location: [source](https://github.com/nayuki/QR-Code-generator/tree/a6ef65d237628a03dee3ae1df592df9a3359204d/javascript)

# Project folders

* folder `lwip-port`: lwip porting files on Windows platform
* folder `straight-httpd`: Microsoft Visual Studio 2017 project: `straight-httpd.sln`
