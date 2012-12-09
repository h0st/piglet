#
#  Makefile for Piglet
#
#  Author: Ora Lassila mailto:ora.lassila@nokia.com
#  Copyright (c) 2008 Nokia. All Rights Reserved.
#

SHELL = /bin/sh

CC = gcc
CXX = g++

OBJ = obj/
SRC = src/

LIBRARY = libpiglet.so
CHEADERS = cpiglet.h
CFLAGSAUX = -fPIC
LDFLAGSAUX =
DYNFLAG = -shared
CFLAGS = -g

ifeq ($(OSTYPE), darwin)
	LIBRARY = libpiglet.dylib
	CFLAGSAUX =
	LDFLAGSAUX = -lgcc_s.1 -arch_only i386
	DYNFLAG = -dynamiclib
endif

INSTALLDIR = /usr/local/lib/
LIBINSTALLDIR = /usr/local/include/

all : library samples pystuff

# CHEADERS added by jhonkola
# TODO: copy also other libraries in 'make install'

install : library pystuff
	cp $(LIBRARY) $(INSTALLDIR)
	cp $(SRC)$(CHEADERS) $(LIBINSTALLDIR)
	python $(SRC)setup.py install

#  Source dependencies

$(SRC)sqlconst.h : $(SRC)makesql.py $(SRC)createDB.sql $(SRC)createTempDB.sql
	$(SRC)makesql.py $(SRC)

$(SRC)Action.h : $(SRC)Triple.h

$(SRC)DB.h : $(SRC)SQL.h $(SRC)Action.h

$(SRC)RaptorParser.h : $(SRC)Parser.h

$(SRC)cpiglet.cpp : $(SRC)cpiglet.h

$(SRC)piglet.h : $(SRC)DB.h

$(OBJ)DB.o : $(SRC)DB.cpp $(SRC)DB.h $(SRC)Useful.h $(SRC)Node.h $(SRC)Condition.h $(SRC)sqlconst.h

$(OBJ)%.o : $(SRC)%.cpp $(SRC)%.h $(SRC)Useful.h $(SRC)Node.h $(SRC)Condition.h
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) $(CFLAGSAUX) -o $@ $<

#  Dynamic library
#
#  Note: At this point, the LDFLAGS are *highly* platform specific...

library : $(LIBRARY)
	@echo "LIBRARY =" $(LIBRARY)

LDFLAGS = -lcurl -lraptor -lsqlite3 -lstdc++ -lc $(LDFLAGSAUX)

libobjects = $(OBJ)Action.o $(OBJ)DB.o $(OBJ)Condition.o $(OBJ)Curl.o $(OBJ)Mutex.o $(OBJ)Node.o \
	     $(OBJ)Parser.o $(OBJ)RaptorParser.o $(OBJ)SQL.o $(OBJ)Triple.o $(OBJ)Useful.o \
	     $(OBJ)cpiglet.o $(OBJ)AQLSupport.o $(OBJ)AQLToSQLTranslator.o $(OBJ)SQLExecutor.o \
	     $(OBJ)AQLDebug.o $(OBJ)AQLModel.o $(OBJ)AQLLispParser.o $(OBJ)AQLQueryExecutor.o \
	     $(OBJ)AQLParser.o

$(LIBRARY) : $(libobjects)
	$(CC) $(DYNFLAG) -o $(LIBRARY) $(LDFLAGS) $(libobjects)

#  Sample programs

samples : c++piglet-sample cpiglet-sample m3-cpiglet-sample aqltester

c++piglet-sample : $(LIBRARY) $(OBJ)c++piglet-main.o
	$(CC) -o c++piglet-sample -L. -lpiglet $(LDFLAGS) $(OBJ)c++piglet-main.o

$(OBJ)c++piglet-main.o : $(SRC)c++piglet-main.cpp $(SRC)piglet.h
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) -o $(OBJ)c++piglet-main.o $(SRC)c++piglet-main.cpp

cpiglet-sample : $(LIBRARY) $(OBJ)cpiglet-main.o
	$(CC) -o cpiglet-sample -L. -lpiglet $(LDFLAGS) $(OBJ)cpiglet-main.o

$(OBJ)cpiglet-main.o : $(SRC)cpiglet-main.c $(SRC)cpiglet.h
	$(CC) -c $(CFLAGS) -o $(OBJ)cpiglet-main.o $(SRC)cpiglet-main.c

m3-cpiglet-sample : $(LIBRARY) $(OBJ)cpiglet-main-m3.o
	$(CC) -o m3-cpiglet-sample -L. -lpiglet $(LDFLAGS) $(OBJ)cpiglet-main-m3.o

$(OBJ)cpiglet-main-m3.o : $(SRC)cpiglet-main-m3.c $(SRC)cpiglet.h
	$(CC) -c $(CFLAGS) -o $(OBJ)cpiglet-main-m3.o $(SRC)cpiglet-main-m3.c

aqltester : $(LIBRARY) $(OBJ)aqltester-main.o
	$(CC) -o aqltester -L. -lpiglet $(LDFLAGS) $(OBJ)aqltester-main.o

$(OBJ)aqltester-main.o : $(SRC)aqltester-main.cpp
	$(CXX) -c $(CFLAGS) $(CPPFLAGS) -o $(OBJ)aqltester-main.o $(SRC)aqltester-main.cpp

#  Python extension

pystuff : library $(SRC)pygletmodule.c $(SRC)setup.py
	python $(SRC)setup.py build --build-base=$(SRC)

#  Housekeeping

clean :
	-rm -f $(LIBRARY) c++piglet-sample cpiglet-sample $(SRC)sqlconst.h \
	$(libobjects) $(OBJ)c++piglet-main.o $(OBJ)cpiglet-main.o \
	$(OBJ)aqltester-main.o $(OBJ)cpiglet-main-m3.o aqltester \
	m3-cpiglet-sample
