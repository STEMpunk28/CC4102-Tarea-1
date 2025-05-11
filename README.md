# CC4102 Tarea 1

- Integrantes:
    - Rodrigo Araya G.
    - Nicolás Gonzalez
    - Sebastián Saez H.

## Encontrar Aridad
- Echamos a andar la imagen de Docker (Powershell de Windows):

```
docker run --rm -it -m 50m -v "${PWD}:/workspace" pabloskewes/cc4102-cpp-env bash
```

- Compilamos el codigo a utilizar

```
g++ -O2 -o ./buscarA ./busqueda_a.cpp
```

- Ejecutamos

```
./buscarA
```

## Realizar experimentación
- Echamos a andar la imagen de Docker (Powershell de Windows):

```
docker run --rm -it -m 50m -v "${PWD}:/workspace" pabloskewes/cc4102-cpp-env bash
```

- Compilamos los codigos a utilizar

```
g++ -O2 -o ./generate ./generatorBlock.cpp
g++ -O2 -o ./check ./check.cpp
g++ -O2 -o ./MergeSort ./MergeSort.cpp
g++ -O2 -o ./QuickSort ./QuickSort.cpp
g++ -O2 -o ./main ./main.cpp
```

- Ejecutamos main

```
./main
```

Este comando realiza la experimentación, donde cada paso se documenta en la terminal.
