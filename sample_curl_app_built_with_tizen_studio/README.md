# sample_curl_app_built_with_tizen_studio

Sample application based on
[url2file](https://curl.haxx.se/libcurl/c/url2file.html)
cURL demo built using Tizen Studio.

## Prerequisites

- Tizen Studio installed and configured according to the [Getting Started](https://developer.samsung.com/smarttv/develop/extension-libraries/webassembly/getting-started.html) guide.

## Building

1. Launch Tizen Studio.

2. Create a project in Tizen Studio:

   - File -> New -> Tizen Project
   - Select 'Template' and click 'Next'
   - Select 'TV' and click 'Next'
   - Select 'Web Application', check 'WebAssembly (C/C++)' box and click 'Next'
   - Select 'Empty Project' and click 'Next'
   - Give a name to your project (eg. HelloCURL) and click 'Next'
   - Provide required Emscripten toolchain paths and click 'Finish'

3. Add `http://tizen.org/privilege/internet` privilege to your application.
   See [Configuring TV Applications](https://developer.samsung.com/smarttv/develop/guides/fundamentals/configuring-tv-applications.html) guide how to do this.

4. Add a new WebAssembly module to your project:

   - Right click on your project in 'Project Explorer'
   - From the context menu select 'New' -> 'WebAssembly Module'
   - Choose 'Empty module' and click 'Next'
   - Give a name to your module (e.g. HelloCURL_module)
   - Add 'TextArea' by selecting 'On' checkbox. Logs written to stdout
     by the WebAssembly module will be displayed on this 'TextArea'.
   - Click 'Finish'

5. Either replace contents of `src/empty.cpp` file in the created WebAssembly
   module with contents of
   [url2file_side_thread.cpp](./url2file_side_thread.cpp) or just copy this
   file to the `src/` directory and delete the `src/empty.cpp` file.

6. Download [CA certificates extracted from Mozilla](https://curl.haxx.se/ca/cacert.pem)
   and save it in the main directory of the WebAssembly module project.

7. Add necessary compiler flag to your WebAssembly module:

   - Right click on your WebAssembly module project
   - Select 'Properties' from the context menu
   - Select 'C/C++ Build' -> 'Settings'
   - On 'Tool Settings' tab select 'Emscripten C++ compiler' -> 'Miscellaneous'
   - Append following flag to 'Other flags':

     ```bash
     -s USE_CURL=1
     ```

     This flag `-s USE_CURL=1` is required here and in linker flags below.
     As a compiler flag it is required to have cURL include dirs populated and
     provided to compilation stage (using `-I` switch). Also putting this flag
     here will allow indexer to see cURL include directories and properly
     resolve cURL includes.

8. Add necessary linker flags to your WebAssembly module:

   - Right click on your WebAssembly module project
   - Select 'Properties' from the context menu
   - Select 'C/C++ Build' -> 'Settings'
   - On 'Tool Settings' tab select 'Emscripten C++ linker' -> 'Miscellaneous'
   - Append following flags to 'Linker flags':

     ```bash
     -s ENVIRONMENT_MAY_BE_TIZEN -s USE_CURL=1 --preload-file ../cacert.pem@/cacert.pem -pthread -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=2
     ```

   Flags description:

   | Flag     | Description  |
   | -------- | ------------ |
   | `-s ENVIRONMENT_MAY_BE_TIZEN` | flag indicating that we want to use Samsung Tizen Emscripten extensions available on Samsung Tizen TVs. This flag is necessary to use POSIX sockets APIs in your application. |
   | `-s USE_CURL=1` | flag indicating that we want to build and use cURL library provided with Samsung modified Tizen Emscripten toolchain. Note that during 1st build full library is being built, so compilation may take a while. However after 1st build cURL library is cached. See more on details on [Emscripten Ports](https://emscripten.org/docs/compiling/Building-Projects.html#emscripten-ports) |
   | `--preload-file ../cacert.pem@/cacert.pem` | this option allows your application to read the cacert.pem file using standard C APIs `fopen(./cacert.pem)`. Tizen Studio launches Emscripten toolchain in a `CurrentBin` directory, however we have the `cacert.pem` file in main project's directory. To avoid copying this file we use relative path. To refer to this file as `/cacert.pem` in C/C++ program we use mapping `@/cacert.pem` as described in [Modifying file locations in the virtual file system](https://emscripten.org/docs/porting/files/packaging_files.html#modifying-file-locations-in-the-virtual-file-system) |
    `-pthread -s USE_PTHREADS=1 -s PTHREAD_POOL_SIZE=2` | this options allow to use threads in WebAssembly module. Note that it is necessary to call POSIX sockets APIs in threads other than main thread of your application. |

9. Build your WebAssembly module project.

10. Apply following workarounds:

    1. Unlink the WebAssembly module from main application. Right click on
       main app, then in 'Others' -> 'Project References' unselect referenced
       WebAssembly module project.

    2. While being on project properties add *.data file to the WGT package,
       in 'Tizen Studio' -> 'Package' expand tree to 'CurrentBin' directory
       of the WebAssembly module project and check file with `data` extension.
       Click 'Apply and Close'. Note this particular step must be performed
       after each build of the WebAssembly module project.

    3. Add following snippet to `wasm_modules/scripts/wasm_tools.js` after
       the body `print` method:

       ```javascript
       locateFile: ( () => {
         return (path, prefix) => {
           if (prefix == '') {
             prefix = this.path.substring(0, this.path.lastIndexOf('/')) || '';
             prefix = prefix + '/';
           }
           return prefix + path;
         };
       })(),
       ```

11. Build signed package.

12. Now you're done you can run your application on the TV.
