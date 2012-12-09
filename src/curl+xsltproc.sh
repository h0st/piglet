#!/bin/sh
curl --location --silent "$1" | xsltproc --nowrite $2 -
