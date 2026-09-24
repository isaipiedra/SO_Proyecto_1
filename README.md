# Proyecto 1 de SO

## Descripción del proyecto

El presente proyecto programada corresponde al primer proyecto asignado para el curso de Principios de Sistemas Operativos y consiste en una herramienta de compresión de archivos de texto plano con el uso del algoritmo de codificación de Huffman, que permite asignar códigos variables de redundancia mínima a una cadena de símbolos dada para comprimir su contenido usando la frecuencia de aparición de cada símbolo en la cadena de entrada.

Por medio de este proyecto, diseñado específicamente para pruebas, la interfaz permite cargar un directorio con contenido de archivos en texto plano que se comprime usando la misma implementación del algoritmo de Huffman pero en variantes: serial, paralela (usando $fork()$ con el IPC de $pipe()$ ) y concurrente (haciendo uso de $pthread$ con exclusión mutua). Una vez creados los archivos comprimidos, el programa permite al usuario cargar el comprimido para su descompresión usando, nuevamente, versiones serial, paralela y concurrente de la función de descompresión. Este proceso permite obtener estadísticas acerca de la compresión y de los tiempos de ejecución de cada implementación. 


## Autores

- Isaí Gabriel Piedra Torres: [@isaipiedra](https://github.com/isaipiedra).
- Xanders Espinoza Guzman: [@XandersEG](https://github.com/XandersEG).
- Justin Aron Reyes Monge: [@JussReyes](https://github.com/JussReyes).

## Uso

Para compilar el proyecto en una máquina Debian 13, ejecute la siguiente lista de pasos antes de la compilación.

1. Abra una terminal y ejecutela con permisos de administrador usando: ```su -```
2. Después de ingresar su contraseña de administración, actualice su sistema ejecutando ``` sudo apt update ```
3. Espere a que finalice la descarga, si se le pregunta directamente si desea continuar con la instalación de determinados paquetes indique que sí.
4. Una vez actualizado el sistema, corra el siguiente comando que instalará las dependencias necesarias para ejecutar el proyecto ```sudo apt install git build-essential pkg-config libgtk-4-dev libssl-dev```, de la misma forma, si se le pide aceptar la instalación proceda ingresando la opción de sí en la terminal.
5. Configure su identidad de git corriendo los comandos ```git config --global user.name "su_usuario"```, y ```git config --global user.email "suemail@example.com"```, cambiando "su_usuario" por la información de su usuario que desea ingresar y "suemail@example.com" por su correo electrónico correspondiente.

Tras realizar este proceso puede cerrar esta terminal de administrador y podrá descargar el archivo .zip disponible en este repositorio o bien, ejecutar el siguiente comando en el directorio en el que desee descargar los archivos fuente para tener acceso al proyeto.

``` 
git clone https://github.com/isaipiedra/SO_Proyecto_1.git
```

Una vez completado la configuración del entorno, puede realizar la compilación del código abriendo una nueva terminal en el directorio del proyecto y ejecutando el comando:

```
make
```

Así, se genera el archivo ejecutable que puede correr usando:

```
./proyecto_1
```

Seguidamente se le abrirá la ventana para iniciar la ejecución del programa. Inicie la compresión seleccionando un directorio de archvios de texto plano que tenga en su máquina e ingrese el nombre que recibirá el archivo comprimido, cuando esté listo, presione el botón de "Compress". Espere un momento a que termine la ejecución, si ve una ventana de tiempo de espera es normal pues el programa está procesando los archivos, permita que la ejecución termine. Seguidamente se le dará la información para que descomprima el archivo, utilice el botón de "Decompress" y la siguiente ventana que se le mostrará serán las estadísticas del programa.
