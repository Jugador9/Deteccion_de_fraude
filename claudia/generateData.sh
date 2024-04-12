#!/bin/bash

# Valores predeterminados de los parámetros
LINEAS=100
SUCURSAL="SU001"
TIPO_OPERACION="COMPRA01"
USUARIOS=10

if [[ $# -eq 0 ]]; then
    echo "Uso: $0 [-l|--lineas <número de líneas>] [-s|--sucursal <sucursal>] [-t|--tipo <tipo de operación>] [-u|--usuarios <número de usuarios>]"
    exit 1
fi

# Parsear los parámetros
while [ $# -gt 0 ]; do
    case "$1" in
        -l|--lineas)
            LINEAS=$2
            shift 2
            ;;
        -s|--sucursal)
            SUCURSAL=$2
            shift 2
            ;;
        -t|--tipo)
            TIPO_OPERACION=$2
            shift 2
            ;;
        -u|--usuarios)
            USUARIOS=$2
            shift 2
            ;;
        *)
            echo "Uso: $0 [-l|--lineas <número de líneas>] [-s|--sucursal <sucursal>] [-t|--tipo <tipo de operación>] [-u|--usuarios <número de usuarios>]"
            exit 1
            ;;
    esac
done

# Generar datos de prueba
contador=1
while [ $contador -le $LINEAS ]; do
    USUARIO="USER$((RANDOM % USUARIOS + 1))"
    MONTO="$((RANDOM % 500 + 1)).0€"
# ESTADO=("Error" "Finalizado" "Correcto")
    ESTADO_INDEX=$((RANDOM % 3))
    ESTADO_OPERACION="Correcto"
# ESTADO_OPERACION=${ESTADO[ESTADO_INDEX]}
    
    echo "OPE$(printf "%03d" $contador);$(date +"%d/%m/%Y %H:%M");$(date +"%d/%m/%Y %H:%M");$USUARIO;$TIPO_OPERACION;1;+$MONTO;$ESTADO_OPERACION"
    
    contador=$((contador + 1))
done