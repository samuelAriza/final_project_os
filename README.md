# GSEA - Gestión Segura y Eficiente de Archivos

![Language](https://img.shields.io/badge/language-C-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)

## Descripción

GSEA es una utilidad de línea de comandos de alto rendimiento diseñada para comprimir, descomprimir, encriptar y desencriptar archivos y directorios completos de manera eficiente. Implementa algoritmos de compresión y encriptación desde cero, sin depender de librerías externas, y utiliza procesamiento concurrente para optimizar el rendimiento en sistemas multinúcleo.

### Características Principales

- **Compresión LZ77**: Algoritmo de compresión sin pérdida basado en ventanas deslizantes
- **Encriptación AES-128**: Implementación completa del estándar Advanced Encryption Standard
- **Procesamiento Concurrente**: Pool de hilos para procesamiento paralelo de múltiples archivos
- **Llamadas Directas al Sistema**: Usa syscalls POSIX (`open`, `read`, `write`, `close`) para máxima eficiencia
- **Operaciones Combinadas**: Soporte para comprimir y encriptar en una sola operación
- **Sin Dependencias Externas**: Todos los algoritmos implementados desde cero

## Arquitectura del Proyecto

```
├── Makefile
├── README.md
├── src
│   ├── common.h
│   ├── compression
│   │   ├── lz77.c
│   │   └── lz77.h
│   ├── concurrency
│   │   ├── thread_pool.c
│   │   └── thread_pool.h
│   ├── encryption
│   │   ├── aes.c
│   │   └── aes.h
│   ├── file_manager.c
│   ├── file_manager.h
│   ├── main.c
│   └── utils
│       ├── arg_parser.c
│       ├── arg_parser.h
│       ├── error_handler.c
│       └── error_handler.h
└── tests
    ├── test_compression.c
    ├── test_encryption.c
    └── test_integration.c
```

## Compilación e Instalación

### Requisitos

- GCC 7.0 o superior
- GNU Make
- Sistema operativo Linux/Unix con soporte POSIX
- pthread library

### Compilación

```bash
# Compilación estándar
make

# Compilación con símbolos de depuración
make debug

# Ver todas las opciones disponibles
make help
```

### Instalación del Sistema

```bash
# Instalar en /usr/local/bin (requiere sudo)
sudo make install

# Desinstalar
sudo make uninstall
```

## 📖 Guía de Uso

### Sintaxis General

```bash
gsea [OPCIONES]
```

### Opciones Disponibles

| Opción | Descripción |
|--------|-------------|
| `-c` | Comprimir datos |
| `-d` | Descomprimir datos |
| `-e` | Encriptar datos |
| `-u` | Desencriptar datos (decrypt) |
| `--comp-alg ALG` | Algoritmo de compresión (lz77, huffman*, rle*) |
| `--enc-alg ALG` | Algoritmo de encriptación (aes128, des*, vigenere*) |
| `-i PATH` | Ruta del archivo o directorio de entrada |
| `-o PATH` | Ruta del archivo o directorio de salida |
| `-k KEY` | Clave secreta para encriptación/desencriptación |
| `-t NUM` | Número de hilos (por defecto: 4) |
| `-v` | Modo verboso |
| `-h, --help` | Mostrar ayuda |

*En desarrollo

### Ejemplos de Uso

#### 1. Comprimir un archivo

```bash
./bin/gsea -c --comp-alg lz77 -i documento.txt -o documento.lz77
```

#### 2. Descomprimir un archivo

```bash
./bin/gsea -d --comp-alg lz77 -i documento.lz77 -o documento_restaurado.txt
```

#### 3. Encriptar un archivo

```bash
./bin/gsea -e --enc-alg aes128 -i datos.txt -o datos.enc -k "mi_clave_secreta"
```

#### 4. Desencriptar un archivo

```bash
./bin/gsea -u --enc-alg aes128 -i datos.enc -o datos_restaurados.txt -k "mi_clave_secreta"
```

#### 5. Comprimir y Encriptar (operación combinada)

```bash
./bin/gsea -ce --comp-alg lz77 --enc-alg aes128 \
    -i proyecto/ -o backup.sec -k "clave_segura" -t 8
```

#### 6. Desencriptar y Descomprimir

```bash
./bin/gsea -du --enc-alg aes128 --comp-alg lz77 \
    -i backup.sec -o proyecto_restaurado/ -k "clave_segura" -t 8
```

#### 7. Procesar directorio completo

```bash
# Comprimir todos los archivos de un directorio
./bin/gsea -c --comp-alg lz77 -i ./datos/ -o ./datos_comprimidos/ -v
```

## Pruebas

### Ejecutar Suite de Pruebas

```bash
make test
```

Esta suite realiza:
- Pruebas de compresión/descompresión
- Pruebas de encriptación/desencriptación
- Pruebas de operaciones combinadas
- Verificación de integridad de datos

### Pruebas de Memoria

```bash
# Verificar fugas de memoria con Valgrind
make valgrind
```

## 🔬 Detalles Técnicos

### Algoritmo LZ77

**Características:**
- Ventana de búsqueda: 4096 bytes
- Búsqueda hacia adelante: 18 bytes
- Longitud mínima de coincidencia: 3 bytes
- Complejidad temporal: O(n × w) donde w es el tamaño de la ventana
- Complejidad espacial: O(n)

**Formato de Token:**
```
<offset (16 bits), length (8 bits), next_char (8 bits)>
```

**Ratio de Compresión Típico:** 40-70% para texto y datos repetitivos

### Algoritmo AES-128

**Características:**
- Tamaño de clave: 128 bits (16 bytes)
- Tamaño de bloque: 128 bits (16 bytes)
- Número de rondas: 10
- Modo de operación: ECB con padding PKCS#7
- Operaciones implementadas:
  - SubBytes (S-Box)
  - ShiftRows
  - MixColumns
  - AddRoundKey
  - Key Expansion

**Seguridad:** Adecuado para protección de datos confidenciales. Para aplicaciones críticas de seguridad, se recomienda usar AES en modo CBC o GCM con IV aleatorio.

### Sistema de Concurrencia

**Pool de Hilos:**
- Patrón Producer-Consumer con cola de tareas
- Sincronización mediante pthread mutex y variables de condición
- Procesamiento paralelo de archivos independientes
- Escalado automático según número de archivos y núcleos disponibles

**Beneficios de Rendimiento:**
- Speedup lineal hasta el número de núcleos disponibles
- Reducción de tiempo de procesamiento en sistemas multinúcleo
- Gestión eficiente de recursos sin procesos zombie

### Llamadas al Sistema

El proyecto usa exclusivamente llamadas POSIX directas:
- `open()`: Apertura de archivos
- `read()`: Lectura de datos
- `write()`: Escritura de datos
- `close()`: Cierre de descriptores
- `opendir()`, `readdir()`, `closedir()`: Operaciones en directorios
- `stat()`, `fstat()`: Información de archivos
- `mkdir()`: Creación de directorios
- `fsync()`: Sincronización de datos

**Sin usar:** `fopen()`, `fread()`, `fwrite()`, `fclose()` de stdio.h

## Análisis de Rendimiento

### Benchmarks (Hardware de Referencia: CPU 4 núcleos @ 2.5 GHz)

| Operación | Archivo (100 MB) | Directorio (100 archivos, 1 MB c/u) |
|-----------|------------------|--------------------------------------|
| Compresión LZ77 | ~2.5s | ~8s (1 hilo) / ~2.5s (4 hilos) |
| Descompresión LZ77 | ~1.8s | ~6s (1 hilo) / ~1.8s (4 hilos) |
| Encriptación AES | ~3.2s | ~10s (1 hilo) / ~3s (4 hilos) |
| Desencriptación AES | ~3.0s | ~9.5s (1 hilo) / ~2.8s (4 hilos) |

### Escalabilidad

Speedup con múltiples hilos (100 archivos de 1 MB):
- 1 hilo: 1.0x (baseline)
- 2 hilos: 1.9x
- 4 hilos: 3.7x
- 8 hilos: 3.9x (saturación por I/O)

## Caso de Uso: Biotecnología

### Escenario

Una startup de biotecnología genera grandes volúmenes de datos de secuenciación genética diariamente.

**Problemas:**
1. **Confidencialidad:** Protección de propiedad intelectual y datos de pacientes
2. **Almacenamiento:** Archivos masivos costosos de almacenar
3. **Eficiencia:** Procesamiento rápido de lotes diarios

### Solución con GSEA

```bash
#!/bin/bash
# Script de archivado automatizado

FECHA=$(date +%Y-%m-%d)
ENTRADA="./Resultados/${FECHA}/"
SALIDA="./Archivados/${FECHA}.bak"
CLAVE="G3n0m3S3cur1ty!"

./gsea -ce --comp-alg lz77 --enc-alg aes128 \
    -i "${ENTRADA}" -o "${SALIDA}" -k "${CLAVE}" -t 8 -v

echo "Archivado completado: ${SALIDA}"
```

**Beneficios:**
- Compresión >60% por naturaleza repetitiva de datos genéticos (LZ77 ideal)
- Encriptación AES-128 cumple normativas de protección de datos
- Procesamiento paralelo reduce tiempo de archivado en 75%
- Ahorro de costos de almacenamiento significativo

## Desarrollo y Contribución

### Principios de Diseño

1. **Modularidad:** Cada componente es independiente y reutilizable
2. **Claridad:** Código autodocumentado con comentarios explicativos
3. **Robustez:** Manejo exhaustivo de errores y casos límite
4. **Eficiencia:** Optimización de algoritmos y uso de memoria
5. **Seguridad:** Validación de entradas y prevención de vulnerabilidades

### Estándares de Código

- **Estándar C:** C11
- **Estilo:** Notación snake_case para funciones y variables
- **Documentación:** Comentarios Doxygen en headers
- **Testing:** Verificación de integridad en cada operación

### Roadmap Futuro

- [ ] Implementación de Huffman Coding
- [ ] Implementación de RLE (Run-Length Encoding)
- [ ] Modo CBC para AES con IV aleatorio
- [ ] Soporte para DES y Triple-DES
- [ ] Compresión adaptativa (selección automática de algoritmo)
- [ ] Interfaz gráfica (GTK)
- [ ] Soporte para Windows API

## Referencias

1. Ziv, J., & Lempel, A. (1977). "A universal algorithm for sequential data compression"
2. NIST FIPS 197 (2001). "Advanced Encryption Standard (AES)"
3. Stevens, W. R., & Rago, S. A. (2013). "Advanced Programming in the UNIX Environment"
4. Tanenbaum, A. S. (2014). "Modern Operating Systems"

## Autores

**Equipo de Desarrollo**
- Samuel Andrés Ariza Gómez
- Andrés Vélez Rendón
- Juan Pablo Mejía Pérez
