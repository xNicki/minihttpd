# MiniHTTPd
Servidor HTTP/1.1 basico en C para Linux, sin bibliotecas HTTP externas.

Proyecto de Computacion Distribuida.

## Características principales

- **Concurrencia event-driven:** Manejo de múltiples clientes simultáneos con `epoll` y conexiones persistentes.
- **Contenido estático:** Soporte para HTML, imágenes y otros recursos mediante detección de tipos MIME.
- **Protocolo HTTP/1.1:** Procesamiento de solicitudes `GET` y encabezados básicos (`Host`, `Connection`, `User-Agent`).
- **Códigos de estado:** `200 OK`, `400 Bad Request`, `403 Forbidden`, `404 Not Found`, `405 Method Not Allowed`, `500 Internal Server Error`.

## Seguridad:
  - Protección contra *Directory Traversal* mediante `realpath()`.
  - Prevención de *Buffer Overflows* evitando funciones inseguras (`strcpy`, `sprintf`).
  - Validación de tamaño máximo de solicitud y rechazo de métodos distintos a `GET`.

## Compilar y ejecutar

```
make
./minihttpd            # puerto 8080, carpeta www

```

Abrir en el navegador: http://localhost:8080

## Pruebas rapidas

```
curl -i http://localhost:8080/                          # 200
curl -i http://localhost:8080/nada.html                 # 404
curl -i --path-as-is http://localhost:8080/../../../../../../../../etc/passwd   # 403
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

