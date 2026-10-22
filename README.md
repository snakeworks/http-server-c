# http-server-c

Simple HTTP server written in C with a custom HTML template syntax (for funsies).

This project is only meant to serve as an example on how to utilize the standard C headers to write an HTTP server. I make no claims about the safety and production readiness of this software.

## Quick Start

```console
$ make
$ ./build/main # run server
```

## TODO
- Add examples for all HTTP methods, not just GET.
- Make safer. Memory isn't managed as good as I'd hoped.
- Extend template syntax to include loops and conditionals.
