REM https://blog.jayway.com/2014/09/03/creating-self-signed-certificates-with-makecert-exe-for-development/

REM to create root CA
makecert.exe -n "CN=CARoot" -r -pe -a sha512 -len 4096 -cy authority -sv CARoot.pvk CARoot.cer
REM pack root CA as pfx
pvk2pfx.exe -pvk CARoot.pvk -spc CARoot.cer -pfx CARoot.pfx -po straight

REM to create server certificate
makecert.exe -n "CN=server.straight" -iv CARoot.pvk -ic CARoot.cer -pe -a sha512 -len 4096 -b 06/06/2020 -e 06/06/2030 -sky exchange -eku 1.3.6.1.5.5.7.3.1 -sv server.pvk server.cer
REM pack server cert as pfx
pvk2pfx.exe -pvk server.pvk -spc server.cer -pfx server.pfx -po straight

REM to create client certificate
makecert.exe -n "CN=client.straight" -iv CARoot.pvk -ic CARoot.cer -pe -a sha512 -len 4096 -b 06/06/2020 -e 06/06/2030 -sky exchange -eku 1.3.6.1.5.5.7.3.2 -sv client.pvk client.cer
REM pack client cert as pfx
pvk2pfx.exe -pvk client.pvk -spc client.cer -pfx client.pfx -po straight

REM openssl pkcs12 -in server.pfx -out server.pem

pause
