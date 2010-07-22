#
# Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
# All rights reserved.
# This component and the accompanying materials are made available
# under the terms of "Eclipse Public License v1.0"
# which accompanies this distribution, and is available
# at the URL "http://www.eclipse.org/legal/epl-v10.html".
#
# Initial Contributors:
# Nokia Corporation - initial contribution.
#
# Contributors:
#
# Description: 
#
#

TEMPLATE = app
TARGET = test_qtnmwrapper
CONFIG += symbian_test

INCLUDEPATH += .
INCLUDEPATH += $$MW_LAYER_SYSTEMINCLUDE \
               /epoc32/include/mw/qt \
               /epoc32/include/mw/qttest \
               ../inc

TARGET.CAPABILITY = ALL -TCB
#DEFINES += _LOG_TO_C_

# Input
LIBS +=     -lthumbnailmanager \
            -lfbscli \
            -lthumbnailmanagerqt

SOURCES +=  test_qtnmwrapper.cpp \
            ../src/thumbnailmanager_p_qt.cpp
            
HEADERS += ../inc/thumbnailmanager_p_qt.h

CONFIG += qtestlib console

icons.sources += tnmwrapper_tsrc.png
icons.sources += tnmwrapper_tsrc.mbm
icons.path = /
DEPLOYMENT += icons
