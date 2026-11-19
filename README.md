# MiniHTTPd

Servidor HTTP/1.1 basico en C para Linux, sin bibliotecas HTTP externas.
Proyecto Integrador de Computacion Distribuida.

## Que hace

- Sirve archivos estaticos de la carpeta `www/` (solo metodo GET).
- Atiende muchos clientes a la vez con un solo hilo usando `epoll`.
- Conexiones persistentes (keep-alive).
- Lee los encabezados `Host`, `Connection` y `User-Agent`.
- Codigos de estado: 200, 400, 403, 404, 405 y 500.
- Tipos MIME: html, css, js, png y jpg.

## Compilar y ejecutar

```
make
./minihttpd            # puerto 8080, carpeta www
./minihttpd 9000 www   # puerto y carpeta opcionales
```

Abrir en el navegador: http://localhost:8080

## Pruebas rapidas

```
curl -i http://localhost:8080/                          # 200
curl -i http://localhost:8080/nada.html                 # 404
curl -i --path-as-is http://localhost:8080/../../etc/passwd   # 403
curl -i -X POST http://localhost:8080/                  # 405
curl -i "http://localhost:8080/$(head -c 3000 /dev/zero | tr '\0' a)"  # 400
```

## Estructura

| Archivo         | Funcion                                              |
|-----------------|------------------------------------------------------|
| `src/main.c`    | Lee puerto y carpeta, inicia el servidor             |
| `src/server.c`  | Bucle de eventos con epoll y manejo de conexiones    |
| `src/http.c`    | Analisis de la solicitud GET y armado de la respuesta|
| `src/files.c`   | Acceso seguro a los archivos                         |
| `src/mime.c`    | Tipo de contenido segun la extension                 |

## Seguridad

- **Directory traversal:** se usa `realpath()` y se comprueba que la ruta
  final siga dentro de `www/` (si no, 403).
- **Buffer overflow:** no se usa `strcpy` ni `sprintf`; solo `memcpy`
  con limites y `snprintf`.
- **Metodos invalidos:** todo lo que no sea GET responde 405.
- **Tamanos maximos:** solicitud 8192 bytes, linea 2048 y URI 1024;
  si se pasan, responde 400.
