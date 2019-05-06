#!/bin/bash

#set -x

TESTING_FOLDER="./testing"
TESTS_FOLDER="./tests"

clear
if [ $# -ne 1 ]; then
	echo "Uso: $0 <ZIP con los fuentes>"
	exit
fi

echo "$1" | grep -q "ssoo_p3_[0-9]*[_]*[0-9]*[_]*[0-9]*.zip"

if [ ! $? -eq 0 ] ;then
	echo "Nombre de fichero incorrecto"
	exit
fi

echo "Nombre de fichero correcto"
echo 
echo "***************************"
echo
ZIP=$(basename "$1")
ZIP="${ZIP%.*}"
unzip $1 -d $ZIP >/dev/null

STUDENTS_CODE=$ZIP
#STUDENTS_CODE=ssoo_p3....
STUDENTS_FILE=$STUDENTS_CODE/lib/concurrency_layer.c
TEST_NO=0


# TEST 0
cd $STUDENTS_CODE 

echo
make clean
echo
make 2> compilation

cat compilation |grep "Error" > compilerr
ERR=$(wc -l < "compilerr")
if [ $ERR -ne 0 ]; then
	echo "***********************"
	echo "Error de compilacion:"
	cat compilation
	cd ..
	rm -r $ZIP
	exit
fi

echo
echo "Compilacion OK"

cd ..

rm -r  $ZIP

