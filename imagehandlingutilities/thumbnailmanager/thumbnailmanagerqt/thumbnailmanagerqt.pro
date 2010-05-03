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

TEMPLATE = lib
TARGET = thumbnailmanagerqt

symbian:TARGET.UID3=0x20029F65

DEFINES+=TNMQT_DLL

INCLUDEPATH += .
INCLUDEPATH += $$MW_LAYER_SYSTEMINCLUDE

TARGET.CAPABILITY = CAP_GENERAL_DLL

symbian:TARGET.EPOCALLOWDLLDATA = 1

# Input
LIBS +=     -lthumbnailmanager.dll \
            -lfbscli.dll


SOURCES += src/thumbnailmanager_qt.cpp \
           src/thumbnailmanager_p_qt.cpp  
          
HEADERS += ../../../imagehandling_plat/thumbnailmanager_api/inc/thumbnailmanager_qt.h \
           inc/thumbnailmanager_p_qt.h  

BLD_INF_RULES.prj_exports += "thumbnailmanagerqt.iby /epoc32/rom/include/core/mw/"

myDefInclude = "NOSTRICTDEF" \
"$${LITERAL_HASH}if defined(WINS)"\
"DEFFILE  bwins/thumbnailmanagerqt.def "\
"$${LITERAL_HASH}else "\
"DEFFILE  eabi/thumbnailmanagerqt.def "\
"$${LITERAL_HASH}endif"
MMP_RULES += myDefInclude

