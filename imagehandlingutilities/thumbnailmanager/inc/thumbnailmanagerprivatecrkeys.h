/*
* Copyright (c) 2006-2007 Nokia Corporation and/or its subsidiary(-ies). 
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:  Central Repository keys for Thumbnail Manager
 *
*/


#ifndef THUMBNAILMANAGERPRIVATECRKEYS_H
#define THUMBNAILMANAGERPRIVATECRKEYS_H

#include "thumbnailmanageruids.hrh"

const TUid KThumbnailSettingsUid = 
    {
    THUMBNAIL_CENREP_UID
    };

const TUint32 KSizeImageGridWidth = 0x0;
const TUint32 KSizeImageGridHeight = 0x1;
const TUint32 KSizeImageListWidth = 0x2;
const TUint32 KSizeImageListHeight = 0x3;
const TUint32 KSizeImageFullscreenWidth = 0x4;
const TUint32 KSizeImageFullscreenHeight = 0x5;
const TUint32 KSizeVideoGridWidth = 0x6;
const TUint32 KSizeVideoGridHeight = 0x7;
const TUint32 KSizeVideoListWidth = 0x8;
const TUint32 KSizeVideoListHeight = 0x9;
const TUint32 KSizeVideoFullscreenWidth = 0x10;
const TUint32 KSizeVideoFullscreenHeight = 0x11;
const TUint32 KSizeAudioGridWidth = 0x12;
const TUint32 KSizeAudioGridHeight = 0x13;
const TUint32 KSizeAudioListWidth = 0x14;
const TUint32 KSizeAudioListHeight = 0x15;
const TUint32 KSizeAudioFullscreenWidth = 0x16;
const TUint32 KSizeAudioFullscreenHeight = 0x17;
const TUint32 KSizeContactListWidth = 0x18;
const TUint32 KSizeContactListHeight = 0x19;
const TUint32 KSizeContactGridWidth = 0x20;
const TUint32 KSizeContactGridHeight = 0x21;
const TUint32 KSizeContactFullscreenWidth = 0x22;
const TUint32 KSizeContactFullscreenHeight = 0x23;

const TUint32 KAutoCreateImageGrid = 0x100;
const TUint32 KAutoCreateImageList = 0x101;
const TUint32 KAutoCreateImageFullscreen = 0x102;
const TUint32 KAutoCreateVideoGrid = 0x103;
const TUint32 KAutoCreateVideoList = 0x104;
const TUint32 KAutoCreateVideoFullscreen = 0x105;
const TUint32 KAutoCreateAudioGrid = 0x106;
const TUint32 KAutoCreateAudioList = 0x107;
const TUint32 KAutoCreateAudioFullscreen = 0x108;
const TUint32 KAutoCreateContactGrid = 0x109;
const TUint32 KAutoCreateContactList = 0x110;
const TUint32 KAutoCreateContactFullscreen = 0x111;

const TUint32 KColorDepthGrid = 0x200;
const TUint32 KColorDepthList = 0x201;
const TUint32 KColorDepthFullscreen = 0x202;

const TUint32 KEnableDaemon = 0x300;

#endif // THUMBNAILMANAGERPRIVATECRKEYS_H
