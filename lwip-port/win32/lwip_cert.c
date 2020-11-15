#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Date: 2020-11-15, created by https://github.com/straight-coding/EmbedTools

//Issuer: CN=Straight RootCA
//Subject: CN=Straight Server

const char *privkey = "-----BEGIN PRIVATE KEY-----\n"\
    "MIIBUwIBADANBgkqhkiG9w0BAQEFAASCAT0wggE5AgEAAkEAhD0FKNdH91c8Vis0\n"\
    "T7Pli3Grb+BM5xA1V/iNTGer5WSwJlAab6lJ6NNh7R15AXOO7XODOs58ikmEqgWi\n"\
    "wacQfwIDAQABAkAG4KeSirPO/OYB80hKtugC2xwX+vn08IZdt2sd5Kxvhzvmp9eM\n"\
    "F4QhlQLHOMrk5LkM7FF0G3FgZHlOAZAVbQTtAiEA6SOLWEpnCCEkkCLMmZTcwzV0\n"\
    "cX9c7ngnOF/xwIn8IT0CIQCRNJVZ3YcJoXFuOCdUid8qOqdatCDkV8TQNxXxPVSc\n"\
    "awIgR1fIMXl7NAKoZK8xeyIRuG7oNj8qWhNMtTSvDyNqk2UCIGgVWi0ldwN3Pviz\n"\
    "tbWKcnYxvv5sedtT8pcRtV/MB5drAiBZSqkW9Ha37EObdrctWBvBvHtUp8k9XOy6\n"\
    "1X0wxUy5BQ==\n"\
    "-----END PRIVATE KEY-----\n";

const char *cert = "-----BEGIN CERTIFICATE-----\n"\
    "MIIB2jCCAYSgAwIBAgIIU3U2E0/GMUowDQYJKoZIhvcNAQELBQAwGjEYMBYGA1UE\n"\
    "AwwPU3RyYWlnaHQgUm9vdENBMB4XDTIwMTExNTAwMDAwMFoXDTQwMTExNTAwMDAw\n"\
    "MFowGjEYMBYGA1UEAwwPU3RyYWlnaHQgU2VydmVyMFwwDQYJKoZIhvcNAQEBBQAD\n"\
    "SwAwSAJBAIQ9BSjXR/dXPFYrNE+z5Ytxq2/gTOcQNVf4jUxnq+VksCZQGm+pSejT\n"\
    "Ye0deQFzju1zgzrOfIpJhKoFosGnEH8CAwEAAaOBrTCBqjBJBgNVHSMEQjBAgBSD\n"\
    "hOKzs+3Mo56OeliOMM0gQZgafKEepBwwGjEYMBYGA1UEAwwPU3RyYWlnaHQgUm9v\n"\
    "dENBgghnEtSASbZ0HDAdBgNVHQ4EFgQUGroKNtRTXQ7nxeYSQlZq35oVQDQwDAYD\n"\
    "VR0TAQH/BAIwADATBgNVHSUEDDAKBggrBgEFBQcDATAbBgNVHREEFDASggZzZXJ2\n"\
    "ZXKCCHN0cmFpZ2h0MA0GCSqGSIb3DQEBCwUAA0EAO02jJwxokR4CeA8DDJqp/9Qk\n"\
    "0dim//+cjVTjxqIgUS5ykNW2CAIRuP5rVyzNv6U02F0q92Vs/754/ep+TyT70w==\n"\
    "-----END CERTIFICATE-----\n";
