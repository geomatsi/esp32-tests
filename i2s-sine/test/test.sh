#!/bin/bash

./test $1 $2 > sine.dat && gnuplot script.gnuplot -persist
