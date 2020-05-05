# sample_curl_app_built_with_cli

Sample application based on
[url2file](https://curl.haxx.se/libcurl/c/url2file.html)
cURL demo built using command-line tools (CLI).

## Prerequisites

- Downloaded [Emscripten toolchain modified by Samsung](https://developer.samsung.com/smarttv/develop/extension-libraries/webassembly/download.html) and activated.

## Compilation

1. Download [CA certificates extracted from Mozilla](https://curl.haxx.se/docs/caextract.html) :

   ```bash
   curl -O https://curl.haxx.se/ca/cacert.pem
   ```

2. Compile [url2file.c](./url2file.c) demo

   ```bash
   emcc -o url2file.html -s ENVIRONMENT_MAY_BE_TIZEN -s USE_CURL=1 --proxy-to-worker --preload-file cacert.pem url2file.c
   ```

   | Option   | Description                                                  |
   | -------- | ------------------------------------------------------------ |
   | `-o url2file.html` | Name of the output, this will make that Emscripten will generate url2file.wasm, url2file.js and url2file.html |
   | `-s ENVIRONMENT_MAY_BE_TIZEN` | Flag indicating that we want to use Samsung Tizen Emscripten extensions available on Samsung Smart TV. This flag is necessary to use POSIX sockets APIs in your application. |
   | `-s USE_CURL=1` | Flag indicating that we want to build and use cURL library provided with Samsung Customized Emscripten. Note that during 1st build full library is being built, so compilation may take a while. However after 1st build cURL library is cached. See more on details on [Emscripten Ports](https://emscripten.org/docs/compiling/Building-Projects.html#emscripten-ports) |
   | `--proxy-to-worker` | This flag is needed to run `main()` function of your application in a Web Worker, because Samsung Tizen Sockets Extensions can only be used in a Web Worker.  |
   | `--preload-file cacert.pem` | This option allows your application to read cacert.pem file using standard C APIs `fopen(./cacert.pem)` |

   Implicitly enabled options:

   | Option   | Description                                                  |
   | -------- | ------------------------------------------------------------ |
   | `-s USE_SSL=1` | Flag enabling SSL library (libssl) from OpenSSL libraries. This flags is implicitly turned on by `-s USE_CURL=1` |
   | `-s USE_CRYPTO=1` | Flag enabling use crypto library (libcrypto) from OpenSSL libraries. This flags is implicitly turned on by `-s USE_SSL=1` |
   | `-s USE_ZLIB-1` | Flag enabling use of zlib compression library. This flag is implicitly turned on by `-s USE_CURL=1` |

   Further information regarding Emscripten build options can be found on
   [Emscripten Compiler Frontend (emcc)](https://emscripten.org/docs/tools_reference/emcc.html#emccdoc)

3. Sign and pack widget using Tizen CLI interface:

   ```bash
   tizen package -t wgt -s <YOUR_CERTIFICATE_PROFILE_NAME> -- .
   ````

   Guide showing how to create certificate profile can be found at [Creating Certificates](https://developer.samsung.com/SmartTV/develop/getting-started/setting-up-sdk/creating-certificates.html)

   More information regarding Tizen CLI interface can be found at [Command Line Interface Commands](https://developer.tizen.org/development/tizen-studio/web-tools/cli)

   *Note:*
   This application needs `http://tizen.org/privilege/internet` privilege, as
   defined in provided [config.xml](./config.xml) file.
   More information regarding the config.xml file format can be found on
   [Developer Samsung](https://developer.tizen.org/ko/development/tizen-studio/web-tools/configuring-your-app/configuration-editor?langredirect=1)

4. Set your TV into developer mode as described in
   [TV Device](https://developer.samsung.com/SmartTV/develop/getting-started/using-sdk/tv-device.html)

5. Connect to the TV using sdb:

   ```bash
   sdb connect <TV_IP>
   ```

   SDB command and its options are described on [Connecting Devices over Smart Development Bridge](https://developer.tizen.org/development/tizen-studio/web-tools/running-and-testing-your-app/sdb)

6. List connected devices:

    ```bash
    sdb devices
    ```

    Sample output:

    ```bash
    List of devices attached
    192.168.13.2:26101      device          0
    ```

7. Install widget on the TV:

    ```bash
    tizen install -n url2file.wgt -t 0
    ```

    *Note:*
    In option `-t 0` value `0` is taken from last column of `sdb devices`
    command output.

8. Run widget on the TV:

   ```bash
   tizen run -p url2file00.curl -t 0
   ```
