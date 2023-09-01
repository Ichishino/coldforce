# Examples

## Builds

* Windows  
Visual Studio ([examples/all_examples.sln](https://github.com/Ichishino/coldforce/tree/master/examples/))

  for wolfSSL
  Add `CO_USE_WOLFSSL` to `C/C++ Preprocessor Definitions` in both co_tls and your project property.

* Linux, macOS  
  cmake

  ```shellsession
  cd build
  cmake ..
  make
  ```

  for wolfSSL

  ```shellsession
  ...
  cmake .. -DTLS_LIB=wolfssl
  ...
  ```

## Run

* Linux, macOS

  ```shellsession
  cd build/examples
  cd http_client
  ./http_client http://www.example.com
  ```
