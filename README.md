Coldforce
========

Coldforce は C言語で作られた、さまざまなネットワークプロトコルに対応したフレームワークです。  
現在対応しているネットワークプロトコルは以下となります。  
全てクライアント、サーバー（マルチクライアント）に対応しております。
* TCP/UDP
* TLS(TCP)
* HTTP/1.1
* HTTP/2
* WebSocket

### 対応OS
* Windows
* Linux
* macOS

### ビルド要件
* C99対応コンパイラ
* pthread (-lpthread) オプション
* TLS, HTTP/1.1, HTTP/2機能を使用する場合は、OpenSSL(-lssl -lcrypt) が必要となります。

### モジュール構成
* **co_core** - アプリケーション基本機能
* **co_net** - ネットワーク基本機能及び、TCP/UDP機能
* **co_tls** - TLS機能
* **co_http** - HTTP/1.1機能
* **co_http2** - HTTP/2機能
* **co_ws** - WebSocket機能

### ビルド方法
* Windows  
Visual Studio のプロジェクトファイル (prj/vs19/coldforce.sln) を使用してください。
* Linux  
CMake を使用してください。
```shellsession
$ cd build
$ cmake ../
$ make
```
* macOS  
XCode のプロジェクトファイルを使用してください。

### 実装サンプル
* 実装の基本構成は以下の様になります。
```C
// app object
typedef struct
{
    co_app_t base_app;

    // Your app data here.

} my_app;

bool on_my_app_create(my_app* self, const co_arg_st* arg)
{
    // Your initialization code here.

    return true;
}

void on_my_app_destroy(my_app* self)
{
    // Your deinitialization code here.
}

int main(int argc, char* argv[])
{
    my_app app;

    co_net_app_init(
        (co_app_t*)&app,
        (co_app_create_fn)on_my_app_create,
        (co_app_destroy_fn)on_my_app_destroy);

    // app start
    int exit_code = co_net_app_start((co_app_t*)&app, argc, argv);

    return exit_code;
}
```

* 詳細は [examples](https://github.com/Ichishino/coldforce/tree/master/examples) を参照してください。

### 今後の予定
* WebSocket (over HTTP/2)
* DTLS
* HTTP/3
* WebTransport?
* その他
